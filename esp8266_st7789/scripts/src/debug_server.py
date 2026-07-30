import argparse
import asyncio
import logging
import os
import sys
import threading
import json
from aiohttp import web, WSMsgType
from devices import Device, ProcessDevice, SerialDevice
from response_parser import ResponseHandler, ResponseParser
from command_creator import CommandSender, WeatherIcon, MoonPhase
from typing_extensions import override

logger = logging.getLogger(__name__)

async def run_blocking(event_loop, callback):
    future = event_loop.create_future()
    def thread_runner():
        try:
            result = callback()
            is_success = True
        except Exception as ex:
            result = ex
            is_success = False
        event_loop.call_soon_threadsafe(future.set_result, (is_success, result))
    thread = threading.Thread(target=thread_runner)
    thread.start()
    is_success, result = await future
    thread.join()
    if is_success:
        return result
    raise result

class CustomResponseHandler(ResponseHandler):
    def __init__(self, websocket, event_loop):
        self.websocket = websocket
        self.event_loop = event_loop
        self.sent_message_tasks = []

    async def wait_messages_sent(self):
        for task in self.sent_message_tasks:
            try:
                await task
            except Exception as ex:
                logger.error(f"Failed to send message: {ex}")

    async def send_json(self, data: dict):
        if self.websocket.closed: return
        task = asyncio.create_task(self.websocket.send_json(data))
        self.sent_message_tasks.append(task)

    async def send_data(self, data: bytearray):
        if self.websocket.closed: return
        task = asyncio.create_task(self.websocket.send_bytes(data))
        self.sent_message_tasks.append(task)

    @override
    async def acknowledge_command(self, header, is_success):
        await self.send_json({
            "type": "acknowledge_command",
            "header": header,
            "is_success": is_success,
        })

    @override
    async def render_status(self, is_busy):
        await self.send_json({
            "type": "render_status",
            "is_busy": is_busy,
        })

    @override
    async def log_message(self, message):
        await self.send_json({
            "type": "log_message",
            "message": message,
        })

    @override
    async def debug_message(self, message):
        await self.send_json({
            "type": "debug_message",
            "message": message,
        })

    @override
    async def debug_frame(self, frame):
        await self.send_json({
            "type": "debug_frame",
            "x_start": frame.x_start,
            "x_end": frame.x_end,
            "y_start": frame.y_start,
            "y_end": frame.y_end,
            "width": frame.width,
            "height": frame.height,
            "x_cursor": frame.x_cursor,
            "y_cursor": frame.y_cursor,
            "brightness": frame.brightness,
            "hardware_reset": frame.hardware_reset,
            "label": frame.label,
        })
        await self.send_data(frame.pixel_data)

class CommandParser:
    def __init__(self, command_sender):
        self.command_sender = command_sender

    async def read_command(self, command):
        try:
            command = json.loads(command)
        except Exception as ex:
            logger.error(f"Failed to parse command: {command}")
            return

        def get_field(field, _type):
            value = command.get(field)
            if value == None:
                raise Exception(f"Command missing field '{field}'")
            if not isinstance(value, _type):
                raise Exception(f"Command field '{field}' expected type '{_type}' but got wrong type '{type(value)}'")
            return value

        try:
            _type = get_field("type", str)
            if _type == "trigger_render":
                await self.command_sender.trigger_render()
            elif _type == "set_screen_brightness":
                screen_brightness = get_field("screen_brightness", int)
                await self.command_sender.set_screen_brightness(screen_brightness)
            elif _type == "set_temperature":
                temperature = get_field("temperature", int)
                await self.command_sender.set_temperature(temperature)
            elif _type == "set_humidity":
                humidity = get_field("humidity", int)
                await self.command_sender.set_humidity(humidity)
            elif _type == "set_time_24_hour":
                time_24_hour = get_field("time_24_hour", int)
                show_24_hour = get_field("show_24_hour", int)
                show_leading_zeros = get_field("show_leading_zeros", int)
                await self.command_sender.set_24_hour_time(time_24_hour, show_24_hour, show_leading_zeros)
            elif _type == "set_wind_kph":
                wind_kph = get_field("wind_kph", int)
                await self.command_sender.set_wind_kph(wind_kph)
            elif _type == "set_location":
                location = get_field("location", str)
                location = location.upper()
                await self.command_sender.set_location(location)
            elif _type == "set_weather_description":
                description = get_field("description", str)
                description = description.upper()
                await self.command_sender.set_weather_description(description)
            elif _type == "set_weather_icon":
                icon = get_field("icon", int)
                icon = WeatherIcon(icon)
                await self.command_sender.set_weather_icon(icon)
            elif _type == "set_moon_phase":
                phase = get_field("phase", int)
                phase = MoonPhase(phase)
                await self.command_sender.set_moon_phase(phase)
            else:
                logger.error(f"Unhandled command type={_type}, command={command}")
        except Exception as ex:
            logger.error(ex)

class App:
    def __init__(self, create_device):
        self.create_device = create_device

    async def websocket_handler(self, request):
        websocket = web.WebSocketResponse()
        await websocket.prepare(request)
        await self.launch_process(websocket)
        return websocket

    async def launch_process(self, websocket):
        event_loop = asyncio.get_running_loop()

        response_handler = CustomResponseHandler(websocket, event_loop)
        response_parser = ResponseParser(response_handler)
        device = await run_blocking(event_loop, lambda: self.create_device(event_loop))
        if device == None:
            logger.error("Failed to create device")
            return
        command_sender = CommandSender(writer=device.write)
        command_parser = CommandParser(command_sender)

        async def device_read_loop():
            while True:
                try:
                    buffer = await device.read()
                    await response_parser.read(buffer)
                except Exception as ex:
                    logger.info(f"Closing async device read loop: {ex}")
                    break

        async def websocket_read_loop():
            async for message in websocket:
                if message.type == WSMsgType.TEXT:
                    await command_parser.read_command(message.data)
                elif message.type == WSMsgType.CLOSE:
                    logger.info("Websocket async read loop closed down")
                    break

        device_read_task = asyncio.create_task(device_read_loop())
        try:
            await websocket_read_loop()
        except Exception as ex:
            logger.error(f"Websocket async read loop failed with: {ex}")

        try:
            await run_blocking(event_loop, lambda: device.close())
        except Exception as ex:
            logger.error(f"Failed to wait for device: {ex}")

        await response_handler.wait_messages_sent()

    async def on_cleanup(self, web_app):
        logger.info("Cleaning up")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", nargs="?", choices=["process", "serial"], default="process")
    parser.add_argument("--executable", default="../build/st7789.exe")
    parser.add_argument("--port", default=None, type=str, help="COM port to connect to")
    parser.add_argument("--baudrate", default=9600, type=int, help="Rate to communicate with device")
    parser.add_argument("--list-ports", action="store_true", help="List all COM ports connected to computer")
    parser.add_argument("--reset", action="store_true", help="Reset arduino on connection")
    parser.add_argument("--static-dirpath", default="./static")
    args = parser.parse_args()

    log_level = os.environ.get("PYTHON_LOG", "INFO").upper()
    logging.basicConfig(level=log_level)

    if args.list_ports:
        import serial.tools.list_ports
        ports = serial.tools.list_ports.comports()
        if len(ports) == 0:
            logger.error("No ports are available to list")
            return 1
        for port in ports:
            print(port.device)
        return 0

    if not os.path.isdir(args.static_dirpath):
        logger.error(f"'{args.static_dirpath}' is not a valid static directory path")
        return 1
    static_dirpath = args.static_dirpath

    if args.mode == "process":
        exec_filepath = os.path.abspath(args.executable)
        if not os.path.isfile(args.executable):
            logger.error(f"'{args.executable}' is not a valid executable")
            return 1
        import subprocess
        def create_device(event_loop):
            try:
                process = subprocess.Popen(
                    [exec_filepath],
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
            except FileNotFoundError as ex:
                logging.error(f"Failed to launch process: {ex}")
                return None
            device = ProcessDevice(process, event_loop)
            return device
    elif args.mode == "serial":
        port_name = args.port
        if port_name is None:
            import serial.tools.list_ports
            ports = serial.tools.list_ports.comports()
            if len(ports) == 0:
                logger.error("No ports available to choose by default")
                return 1
            port = ports[0]
            logger.info(f"Choosing port '{port.device}' by default")
            port_name = port.device

        import serial
        def create_device(event_loop):
            try:
                ser = serial.Serial()
                ser.port = port_name
                ser.baudrate = args.baudrate
                ser.dtr = args.reset
                ser.open()
            except Exception as ex:
                logger.error(f"Failed to open serial device '{port_name}': {ex}")
                return None

            device = SerialDevice(ser, event_loop)
            return device
    else:
        logger.error(f"Unknown device mode: {args.mode}")
        return 1

    app = App(create_device)
    web_app = web.Application()
    web_app.on_cleanup.append(app.on_cleanup)
    web_app.router.add_get("/", lambda request: web.HTTPFound("/index.html"))
    web_app.router.add_get("/websocket", app.websocket_handler)
    web_app.router.add_static("/", path=static_dirpath, follow_symlinks=True, append_version=True)
    web.run_app(web_app, port=8080)
    return 0


if __name__ == "__main__":
    rv = main()
    sys.exit(rv)
