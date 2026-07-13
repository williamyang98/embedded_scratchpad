import argparse
import logging
import serial
import serial.tools.list_ports
import sys
import os
import time
import json
import requests
import ephem
import math
import functools
import threading
from datetime import datetime
from devices import Device, ProcessDevice, SerialDevice
from response_parser import ResponseHandler
from command_creator import WeatherIcon, MoonPhase
from wmo_weather_codes import WMO_WEATHER_CODES

logger = logging.getLogger(__name__)

class RenderFence:
    def __init__(self, initial_is_busy=False):
        self.cv = threading.Condition()
        self.is_busy = initial_is_busy

    def wait_not_busy(self):
        self.cv.acquire()
        while self.is_busy:
            logger.info("Waiting for render to finish")
            self.cv.wait()
        self.cv.release()

    def set_is_busy(self, is_busy):
        self.cv.acquire()
        is_changed = self.is_busy != is_busy
        self.is_busy = is_busy
        if is_changed:
            self.cv.notify_all()
        self.cv.release()

    def close(self):
        self.set_is_busy(False)

class CustomResponseHandler(ResponseHandler):
    def __init__(self, render_fence):
        self.render_fence = render_fence

    def acknowledge_command(self, header, is_success):
        pass

    def render_status(self, is_busy):
        self.render_fence.set_is_busy(is_busy)

    def log_message(self, message):
        pass

    def debug_message(self, message):
        pass

    def debug_frame(self, frame):
        pass

def get_openmeteo_url(latitude, longitude):
    assert isinstance(latitude, float)
    assert isinstance(longitude, float)
    BASE_URL = "https://api.open-meteo.com/v1/forecast"
    LOCATION_QUERY = f"latitude={latitude:.6f}&longitude={longitude:.6f}"
    PARAMETERS = ["temperature_2m", "relative_humidity_2m", "wind_speed_10m", "weather_code"]
    return f"{BASE_URL}?{LOCATION_QUERY}&current={','.join(PARAMETERS)}"

def graceful_fail(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception as ex:
            logger.error(f"{func.__name__} failed with: {ex}")
    return wrapper

def trigger_every(n, is_immediate=True):
    def decorator(func):
        counter = 0
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            nonlocal counter
            nonlocal is_immediate
            is_trigger = counter == n or is_immediate
            if is_trigger:
                if is_immediate:
                    logger.info(f"Triggering {func.__name__} immediately")
                    is_immediate = False
                else:
                    logger.info(f"Triggering {func.__name__} after {n} calls")
                    counter = 0
                func(*args, **kwargs)
            counter += 1
        return wrapper
    return decorator


def wait_render_fence(func):
    @functools.wraps(func)
    def wrapper(self, *args, **kwargs):
        self.render_fence.wait_not_busy()
        return func(self, *args, **kwargs)
    return wrapper

class Server:
    def __init__(self, device, render_fence, openmeteo_url, location):
        self.device = device
        self.render_fence = render_fence
        self.openmeteo_url = openmeteo_url
        self.command_sender = self.device.get_command_sender()
        self.location = location

    def run(self):
        self.update_location()
        while True:
            self.update_time()
            self.update_weather()
            self.update_moon()
            self.trigger_render()
            self.wait_until_next_minute()

    def wait_until_next_minute(self):
        now = datetime.now()
        margin = 2
        delay = 60-now.second+margin
        time.sleep(delay)

    @graceful_fail
    @wait_render_fence
    def update_location(self):
        self.command_sender.set_location(self.location.upper())

    @graceful_fail
    @wait_render_fence
    def update_time(self):
        now = datetime.now()
        time_24_hour = now.hour*100 + now.minute
        self.command_sender.set_24_hour_time(time_24_hour, False, True)

    @trigger_every(60)
    @graceful_fail
    @wait_render_fence
    def update_weather(self):
        response = requests.get(self.openmeteo_url)
        if response.status_code != 200:
            logger.error(f"Got a bad response from openmeteo with code={response.status_code}")
            return
        data = json.loads(response.text)
        current = data.get("current", None)
        if current == None:
            logger.error(f"Missing current data")
            return
        assert isinstance(current, dict)

        temperature = current.get("temperature_2m", None)
        humidity = current.get("relative_humidity_2m", None)
        wind_speed = current.get("wind_speed_10m", None)
        wmo_weather_code = current.get("weather_code", None)

        if temperature != None:
            temperature = int(round(temperature*10))
            self.command_sender.set_temperature(temperature)
        if humidity != None:
            humidity = int(round(humidity*10))
            humidity = max(humidity, 0)
            self.command_sender.set_humidity(humidity)
        if wind_speed != None:
            wind_speed = int(round(wind_speed*10))
            wind_speed = max(wind_speed, 0)
            self.command_sender.set_wind_kph(wind_speed)
        if wmo_weather_code != None:
            weather_code = WMO_WEATHER_CODES.get(wmo_weather_code, None)
            if weather_code != None:
                self.command_sender.set_weather_description(weather_code.description.upper())
                self.command_sender.set_weather_icon(weather_code.weather_icon)
            else:
                logger.warning(f"Failed to fetch wmo weather code: {wmo_weather_code}")

    @trigger_every(60)
    @graceful_fail
    @wait_render_fence
    def update_moon(self):
        date = ephem.Date(datetime.now())
        next_new_moon = ephem.next_new_moon(date)
        previous_new_moon = ephem.previous_new_moon(date)
        lunation = (date-previous_new_moon)/(next_new_moon-previous_new_moon)
        TOTAL_MOON_PHASES = 8
        phase_index = int(math.floor(lunation*TOTAL_MOON_PHASES)) % TOTAL_MOON_PHASES 
        phase = MoonPhase(phase_index)
        self.command_sender.set_moon_phase(phase)

    @graceful_fail
    @wait_render_fence
    def trigger_render(self):
        self.command_sender.trigger_render()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--latitude", default=-33.857504, type=float, help="Location latitude")
    parser.add_argument("--longitude", default=151.215263, type=float, help="Location longitude")
    parser.add_argument("--location", default="Sydney Opera House", type=str, help="Location description")
    parser.add_argument("--port", default=None, type=str, help="COM port to connect to")
    parser.add_argument("--baudrate", default=9600, type=int, help="Rate to communicate with device")
    parser.add_argument("--list-ports", action="store_true", help="List all COM ports connected to computer")
    parser.add_argument("--no-reset", action="store_true", help="Don't reset arduino on connection")
    args = parser.parse_args()

    log_level = os.environ.get("PYTHON_LOG", "INFO").upper()
    logging.basicConfig(level=log_level)

    if args.list_ports:
        ports = serial.tools.list_ports.comports()
        if len(ports) == 0:
            logger.error("No ports are available to list")
            return 1
        for port in ports:
            print(port.device)
        return 0

    port_name = args.port
    if port_name is None:
        ports = serial.tools.list_ports.comports()
        if len(ports) == 0:
            logger.error("No ports available to choose by default")
            return 1
        port = ports[0]
        logger.info(f"Choosing port '{port.device}' by default")
        port_name = port.device

    if not args.no_reset:
        logger.info("Resetting on connection")

    try:
        ser = serial.Serial()
        ser.port = port_name
        ser.baudrate = args.baudrate
        ser.dtr = not args.no_reset
        ser.open()
    except Exception as ex:
        logger.error(f"Failed to open serial device '{port_name}': {ex}")
        return 1

    try:
        initial_is_busy = not args.no_reset
        render_fence = RenderFence(initial_is_busy)
        response_handler = CustomResponseHandler(render_fence)
        device = SerialDevice(ser, response_handler)
        openmeteo_url = get_openmeteo_url(args.latitude, args.longitude)
        server = Server(device, render_fence, openmeteo_url, args.location)
        server.run()
    except KeyboardInterrupt:
        logger.info("Exiting on keyboard interrupt...")
    except Exception as ex:
        logger.error(f"Server failed with exception: {ex}")
        return 1
    finally:
        render_fence.close()
        device.wait()

    return 0

if __name__ == "__main__":
    rv = main()
    sys.exit(rv)
