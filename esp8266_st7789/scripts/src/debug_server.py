import argparse
import asyncio
import logging
import os
import sys
import threading
from aiohttp import web, WSMsgType
from devices import add_device_argument_subparsers, get_device_factory_from_args
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

class App:
    def __init__(self, device_factory):
        self.device_factory = device_factory

    async def websocket_handler(self, request):
        websocket = web.WebSocketResponse()
        await websocket.prepare(request)
        await self.launch_process(websocket)
        return websocket

    async def launch_process(self, websocket):
        try:
            device = await self.device_factory.create_device()
        except Exception as ex:
            logger.error(f"Failed to create device: {ex}")
            return

        async def device_read_loop():
            while True:
                try:
                    buffer = await device.read()
                    assert isinstance(buffer, bytearray), f"Buffer should be of type bytearray but was '{type(buffer)}'"
                    await websocket.send_bytes(buffer)
                except Exception as ex:
                    logger.info(f"Closing async device read loop: {ex}")
                    break
            await websocket.close()

        device_read_task = asyncio.create_task(device_read_loop())

        try:
            async for message in websocket:
                if message.type == WSMsgType.TEXT:
                    logger.info(f"Got message from websocket: {message.data}")
                elif message.type == WSMsgType.BINARY:
                    data = bytearray(message.data)
                    await device.write(data)
                elif message.type == WSMsgType.CLOSE:
                    logger.info("Websocket async read loop closed down")
                    break
        except Exception as ex:
            logger.error(f"Websocket async read loop failed with: {ex}")

        try:
            await device.close()
            await device_read_task
        except Exception as ex:
            logger.error(f"Failed to wait for device: {ex}")

        logger.info("Ending websocket session")

    async def on_cleanup(self, web_app):
        logger.info("Cleaning up")

def main():
    log_level = os.environ.get("PYTHON_LOG", "INFO").upper()
    logging.basicConfig(level=log_level)

    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="mode", required=True, help="Type of device to create")
    parser.add_argument("--static-dirpath", default=os.getenv("STATIC_FILES_DIR", "./static"))
    add_device_argument_subparsers(subparsers)
    args = parser.parse_args()

    if not os.path.isdir(args.static_dirpath):
        logger.error(f"'{args.static_dirpath}' is not a valid static directory path")
        return 1

    device_res = get_device_factory_from_args(args.mode, args)
    if device_res["type"] == "exit_code":
        return device_res["exit_code"]

    assert device_res["type"] == "device_factory", f"Unknown device result return type: {device_res['type']}"
    device_factory = device_res["device_factory"]

    app = App(device_factory)
    web_app = web.Application()
    web_app.on_cleanup.append(app.on_cleanup)
    # main/main.cpp in g_websocket.uri
    web_app.router.add_get("/api/v1/websocket", app.websocket_handler)
    web_app.router.add_get("/", lambda request: web.HTTPFound("/index.html"))
    web_app.router.add_static("/", path=args.static_dirpath, follow_symlinks=True, append_version=True)
    web.run_app(web_app, port=8080)
    return 0


if __name__ == "__main__":
    rv = main()
    sys.exit(rv)
