import argparse
import logging
import sys
import os
import json
import requests
import ephem
import math
import functools
import asyncio
from datetime import datetime
from devices import add_device_argument_subparsers, get_device_factory_from_args
from response_parser import ResponseHandler, ResponseParser
from command_creator import CommandSender, WeatherIcon, MoonPhase
from wmo_weather_codes import WMO_WEATHER_CODES
from typing_extensions import override

logger = logging.getLogger(__name__)

class RenderFence:
    def __init__(self, initial_is_busy=False):
        self.cv = asyncio.Condition()
        self.is_busy = initial_is_busy

    async def wait_not_busy(self):
        await self.cv.acquire()
        while self.is_busy:
            logger.info("Waiting for render to finish...")
            await self.cv.wait()
            logger.info("Finished waiting for render to finish")
        self.cv.release()

    async def set_is_busy(self, is_busy):
        await self.cv.acquire()
        is_changed = self.is_busy != is_busy
        self.is_busy = is_busy
        if is_changed:
            self.cv.notify_all()
        self.cv.release()

    async def close(self):
        await self.set_is_busy(False)

class CustomResponseHandler(ResponseHandler):
    def __init__(self, render_fence):
        self.render_fence = render_fence

    @override
    async def acknowledge_command(self, header, is_success):
        pass

    @override
    async def render_status(self, is_busy):
        await self.render_fence.set_is_busy(is_busy)

    @override
    async def log_message(self, message):
        logger.info(f"Got message: {message}")

    @override
    async def debug_message(self, message):
        pass

    @override
    async def debug_frame(self, frame):
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
    async def wrapper(self, *args, **kwargs):
        try:
            await func(self, *args, **kwargs)
        except Exception as ex:
            logger.error(f"{func.__name__} failed with: {ex}")
    return wrapper

class TriggerEveryCounter:
    def __init__(self, threshold: int, is_immediate: bool):
        self.is_immediate = is_immediate
        self.threshold = threshold 
        self.counter = 0

    def increment(self):
        is_trigger = self.counter == self.threshold or self.is_immediate
        is_immediate = self.is_immediate
        self.is_immediate = False
        if is_trigger:
            self.counter = 0
        self.counter += 1
        return is_trigger, is_immediate

def trigger_every(n, is_immediate=True):
    def decorator(func):
        trigger_every_name = f"__{func.__name__}__trigger_every_{n}__"
        @functools.wraps(func)
        async def wrapper(self, *args, **kwargs):
            if not hasattr(self, trigger_every_name):
                nonlocal n
                nonlocal is_immediate
                counter = TriggerEveryCounter(n, is_immediate)
                setattr(self, trigger_every_name, counter)
            else:
                counter = getattr(self, trigger_every_name)
            is_trigger, is_immediate = counter.increment()
            if is_trigger:
                if is_immediate:
                    logger.info(f"Triggering {func.__name__} immediately")
                else:
                    logger.info(f"Triggering {func.__name__} after {counter.threshold} calls")
                await func(self, *args, **kwargs)
        return wrapper
    return decorator


def wait_render_fence(func):
    @functools.wraps(func)
    async def wrapper(self, *args, **kwargs):
        assert hasattr(self, "render_fence"), "Server instance must have a 'render_fence' attribute"
        await self.render_fence.wait_not_busy()
        return await func(self, *args, **kwargs)
    return wrapper

class Server:
    def __init__(self, device, render_fence, openmeteo_url, location, screen_brightness):
        self.device = device
        self.render_fence = render_fence
        self.response_handler = CustomResponseHandler(self.render_fence)
        self.response_parser = ResponseParser(self.response_handler)
        self.command_sender = CommandSender(writer=self.device.write)

        self.openmeteo_url = openmeteo_url
        self.location = location
        self.screen_brightness = screen_brightness

    async def run(self):
        logger.info("Starting server...")
        async def device_read_loop():
            while True:
                try:
                    buffer = await self.device.read()
                    await self.response_parser.read(buffer)
                except Exception as ex:
                    logger.info(f"Closing async device read loop: {ex}")
                    break

        async def server_loop():
            await self.on_connection()
            while True:
                await self.update_time()
                await self.update_weather()
                await self.update_moon()
                await self.trigger_render()
                await self.wait_until_next_minute()

        server_task = asyncio.create_task(server_loop())
        logger.info("Server is now running")
        await device_read_loop()
        logger.info("Closing down server...")
        server_task.cancel()
        await self.device.close()
        logger.info("Server closed down")

    async def wait_until_next_minute(self):
        now = datetime.now()
        margin = 2
        delay = 60-now.second+margin
        await asyncio.sleep(delay)

    @graceful_fail
    @wait_render_fence
    async def on_connection(self):
        await self.command_sender.set_location(self.location.upper())
        await self.command_sender.set_screen_brightness(self.screen_brightness)

    @graceful_fail
    @wait_render_fence
    async def update_time(self):
        now = datetime.now()
        time_24_hour = now.hour*100 + now.minute
        await self.command_sender.set_24_hour_time(time_24_hour, False, True)

    @trigger_every(60)
    @graceful_fail
    @wait_render_fence
    async def update_weather(self):
        response = await asyncio.to_thread(requests.get, self.openmeteo_url)
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
            await self.command_sender.set_temperature(temperature)
        if humidity != None:
            humidity = int(round(humidity*10))
            humidity = max(humidity, 0)
            await self.command_sender.set_humidity(humidity)
        if wind_speed != None:
            wind_speed = int(round(wind_speed*10))
            wind_speed = max(wind_speed, 0)
            await self.command_sender.set_wind_kph(wind_speed)
        if wmo_weather_code != None:
            weather_code = WMO_WEATHER_CODES.get(wmo_weather_code, None)
            if weather_code != None:
                await self.command_sender.set_weather_description(weather_code.description.upper())
                await self.command_sender.set_weather_icon(weather_code.weather_icon)
            else:
                logger.warning(f"Failed to fetch wmo weather code: {wmo_weather_code}")

    @trigger_every(60)
    @graceful_fail
    @wait_render_fence
    async def update_moon(self):
        date = ephem.Date(datetime.now())
        next_new_moon = ephem.next_new_moon(date)
        previous_new_moon = ephem.previous_new_moon(date)
        lunation = (date-previous_new_moon)/(next_new_moon-previous_new_moon)
        TOTAL_MOON_PHASES = 8
        phase_index = int(math.floor(lunation*TOTAL_MOON_PHASES)) % TOTAL_MOON_PHASES 
        phase = MoonPhase(phase_index)
        await self.command_sender.set_moon_phase(phase)

    @graceful_fail
    @wait_render_fence
    async def trigger_render(self):
        await self.command_sender.trigger_render()

async def main():
    log_level = os.environ.get("PYTHON_LOG", "INFO").upper()
    logging.basicConfig(level=log_level)

    parser = argparse.ArgumentParser()
    parser.add_argument("--latitude", default=-33.857504, type=float, help="Location latitude")
    parser.add_argument("--longitude", default=151.215263, type=float, help="Location longitude")
    parser.add_argument("--location", default="Sydney Opera House", type=str, help="Location description")
    parser.add_argument("--screen-brightness", default=50, type=int, help="Screen brightness")
    subparsers = parser.add_subparsers(dest="mode", required=True, help="Type of device to create")
    add_device_argument_subparsers(subparsers)
    args = parser.parse_args()

    device_res = get_device_factory_from_args(args.mode, args)
    if device_res["type"] == "exit_code":
        return device_res["exit_code"]

    assert device_res["type"] == "device_factory", f"Unknown device result return type: {device_res['type']}"
    device_factory = device_res["device_factory"]

    while True:
        try:
            render_fence = RenderFence()
            device = None
            device = await device_factory.create_device()
            openmeteo_url = get_openmeteo_url(args.latitude, args.longitude)
            server = Server(device, render_fence, openmeteo_url, args.location, args.screen_brightness)
            await server.run()
        except KeyboardInterrupt:
            logger.info("Exiting on keyboard interrupt...")
            break
        except Exception as ex:
            logger.error(f"Server failed with exception: {ex}. Attempting to reconnect...")
        finally:
            await render_fence.close()
            if device != None:
                await device.close()
            await asyncio.sleep(1)

    return 0

if __name__ == "__main__":
    rv = asyncio.run(main())
    sys.exit(rv)
