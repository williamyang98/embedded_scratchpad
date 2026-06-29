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

logger = logging.getLogger(__name__)

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
            proc = subprocess.Popen(
                [self.exec_filepath],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
        except FileNotFoundError as ex:
            logging.error(f"Failed to launch process: {ex}")
            return

        loop = asyncio.get_running_loop()
        sent_message_futures = []
        def track_message_futures(func):
            def tracked_func(*args, **kwargs):
                concurrent_future = func(*args, **kwargs)
                sent_message_futures.append(concurrent_future)
                return concurrent_future
            return tracked_func

        @track_message_futures
        def send_json(data: dict):
            return asyncio.run_coroutine_threadsafe(websocket.send_json(data), loop)

        @track_message_futures
        def send_data(data: bytearray):
            return asyncio.run_coroutine_threadsafe(websocket.send_bytes(data), loop)

        future_parse_stdout = loop.create_future()
        def parse_stdout():
            while True:
                if websocket.closed:
                    logger.info(f"Exiting early since websocket closed")
                    break

                header_data = proc.stdout.read(4*9)
                if len(header_data) != 36:
                    break
                magic_number, x_start, x_end, y_start, y_end, write_index, image_width, image_height, label_length = struct.unpack("=IiiiiiiiI", header_data)
                MAGIC_NUMBER = 0xDEADBEEF
                if magic_number != MAGIC_NUMBER:
                    logger.error(f"Got invalid header magic number {hex(magic_number)} instead of {hex(MAGIC_NUMBER)}")
                    continue

                if label_length > 0:
                    label_data = proc.stdout.read(label_length)
                    if len(label_data) != label_length:
                        logger.error(f"Missing label data expected {label_length} bytes but only got {len(label_data)} bytes")
                        continue
                    label = label_data.decode("utf-8")
                else:
                    label = None

                image_size = image_width*image_height
                pixel_size_bytes = 2
                image_size_bytes = image_size*pixel_size_bytes
                image_data = proc.stdout.read(image_size_bytes)
                if len(image_data) != image_size_bytes:
                    logger.error(f"Missing image data width={image_width}, height={image_height}, read_bytes={len(image_data)}/{image_size_bytes}")
                    continue
                image_data = memoryview(image_data).cast("H")
                logger.info(f"Read image data: {len(image_data)} bytes")
                data = {
                    "x_start": x_start,
                    "x_end": x_end,
                    "y_start": y_start,
                    "y_end": y_end,
                    "image_width": image_width,
                    "image_height": image_height,
                    "write_index": write_index,
                    "label": label,
                }
                send_json(data)
                send_data(image_data)
            loop.call_soon_threadsafe(future_parse_stdout.set_result, None)

        thread_stdout = threading.Thread(target=parse_stdout)
        thread_stdout.start()
        await future_parse_stdout
        thread_stdout.join()

        for concurrent_future in sent_message_futures:
            try:
                async_future = asyncio.wrap_future(concurrent_future)
                await async_future
            except Exception as ex:
                logger.error(f"Failed to sent message: {ex}")

    async def on_cleanup(self, web_app):
        logger.info("Cleaning up")
        pass


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", nargs="?", default="../build/st7789.exe")
    parser.add_argument("--static-dirpath", default="./static")
    args = parser.parse_args()

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
