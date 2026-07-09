import argparse
import asyncio
import logging
import os
import sys
import threading
from aiohttp import web
from devices import Device, ProcessDevice, SerialDevice
from response_parser import ResponseHandler

logger = logging.getLogger(__name__)

class CustomResponseHandler(ResponseHandler):
    def __init__(self, websocket, event_loop):
        self.websocket = websocket
        self.event_loop = event_loop
        self.sent_message_futures = []

    def run_coroutine(self, coro):
        future = asyncio.run_coroutine_threadsafe(coro, self.event_loop)
        self.sent_message_futures.append(future)

    async def wait_messages_sent(self):
        for concurrent_future in self.sent_message_futures:
            try:
                async_future = asyncio.wrap_future(concurrent_future)
                await async_future
            except Exception as ex:
                logger.error(f"Failed to send message: {ex}")

    def send_json(self, data: dict):
        if self.websocket.closed: return
        self.run_coroutine(self.websocket.send_json(data))

    def send_data(self, data: bytearray):
        if self.websocket.closed: return
        self.run_coroutine(self.websocket.send_bytes(data))

    def acknowledge_command(self, header, is_success):
        logger.info(f"> Acknowledged received: header=0x{header:02X}, is_success={is_success}")

    def render_status(self, is_busy):
        logger.info(f"> Render status: is_busy={is_busy}")

    def log_message(self, message):
        logger.info(f"> Logged Message: {message}")

    def debug_message(self, message):
        logger.info(f"> Debug Message: {message}")

    def debug_frame(self, frame):
        logger.info(f"> Frame: width={frame.width}, height={frame.height}, label='{frame.label}'")
        data = {
            "x_start": frame.x_start,
            "x_end": frame.x_end,
            "y_start": frame.y_start,
            "y_end": frame.y_end,
            "image_width": frame.width,
            "image_height": frame.height,
            "x_cursor": frame.x_cursor,
            "y_cursor": frame.y_cursor,
            "time_nanos": 0,
            "brightness": frame.brightness,
            "hardware_reset": frame.hardware_reset,
            "label": frame.label,
        }
        self.send_json(data)
        self.send_data(frame.pixel_data)

async def run_blocking(event_loop, callback):
    future = event_loop.create_future()
    def thread_runner():
        result = callback()
        event_loop.call_soon_threadsafe(future.set_result, result)
    thread = threading.Thread(target=thread_runner)
    thread.start()
    result = await future
    thread.join()
    return result

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
        device = await run_blocking(event_loop, lambda: self.create_device(response_handler))
        if device == None:
            return
        command_sender = device.get_command_sender()

        temperature = 100
        async for message in websocket:
            logger.info(f"Got message from websocket: {message}")
            def action():
                command_sender.set_temperature(temperature)
                command_sender.trigger_render()
            await run_blocking(event_loop, action)
            temperature += 11

        await run_blocking(event_loop, lambda: device.wait())
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
        def create_device(response_handler):
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
            device = ProcessDevice(process, response_handler)
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
        def create_device(response_handler):
            ser = serial.Serial()
            ser.port = port_name
            ser.baudrate = args.baudrate
            ser.dtr = args.reset
            ser.open()

            device = SerialDevice(ser, response_handler)
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
