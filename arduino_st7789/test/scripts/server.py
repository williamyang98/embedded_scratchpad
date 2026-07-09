import argparse
import asyncio
import logging
import os
import struct
import subprocess
import sys
import threading
import time
from aiohttp import web

from devices import Device, ProcessDevice
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

    async def wait_messages(self):
        for concurrent_future in self.sent_message_futures:
            try:
                async_future = asyncio.wrap_future(concurrent_future)
                await async_future
            except Exception as ex:
                logger.error(f"Failed to sent message: {ex}")

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
    def __init__(self, exec_filepath):
        self.exec_filepath = exec_filepath

    async def websocket_handler(self, request):
        websocket = web.WebSocketResponse()
        await websocket.prepare(request)
        await self.launch_process(websocket)
        return websocket

    async def launch_process(self, websocket):
        try:
            process = subprocess.Popen(
                [self.exec_filepath],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
        except FileNotFoundError as ex:
            logging.error(f"Failed to launch process: {ex}")
            return

        event_loop = asyncio.get_running_loop()
        response_handler = CustomResponseHandler(websocket, event_loop)
        device = ProcessDevice(process, response_handler)
        command_sender = device.get_command_sender()

        await run_blocking(event_loop, lambda: device.wait())
        await response_handler.wait_messages()

    async def on_cleanup(self, web_app):
        logger.info("Cleaning up")
        pass

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", nargs="?", default="../build/st7789.exe")
    parser.add_argument("--static-dirpath", default="./static")
    args = parser.parse_args()

    log_level = os.environ.get("PYTHON_LOG", "INFO").upper()
    logging.basicConfig(level=log_level)

    if not os.path.isfile(args.executable):
        logger.error(f"'{args.executable}' is not a valid executable")
        return 1

    if not os.path.isdir(args.static_dirpath):
        logger.error(f"'{args.static_dirpath}' is not a valid static directory path")
        return 1

    exec_filepath = os.path.abspath(args.executable)
    static_dirpath = args.static_dirpath

    app = App(exec_filepath)
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
