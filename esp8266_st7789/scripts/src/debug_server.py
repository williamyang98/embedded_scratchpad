import argparse
import asyncio
import logging
import os
import sys
import threading
from aiohttp import web, WSMsgType
from devices import Device, ProcessDevice, SerialDevice
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
    def __init__(self, create_device):
        self.create_device = create_device

    async def websocket_handler(self, request):
        websocket = web.WebSocketResponse()
        await websocket.prepare(request)
        await self.launch_process(websocket)
        return websocket

    async def launch_process(self, websocket):
        event_loop = asyncio.get_running_loop()

        device = await run_blocking(event_loop, lambda: self.create_device(event_loop))
        if device == None:
            logger.error("Failed to create device")
            return

        async def device_read_loop():
            while True:
                try:
                    buffer = await device.read()
                    assert isinstance(buffer, bytearray)
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
            await run_blocking(event_loop, lambda: device.close())
            await device_read_task
        except Exception as ex:
            logger.error(f"Failed to wait for device: {ex}")

        logger.info("Ending websocket session")

    async def on_cleanup(self, web_app):
        logger.info("Cleaning up")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", nargs="?", choices=["process", "serial"], default="process")
    parser.add_argument("--executable", default="./build/tests/st7789")
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
    # main/main.cpp in g_websocket.uri
    web_app.router.add_get("/api/v1/websocket", app.websocket_handler)
    web_app.router.add_static("/", path=static_dirpath, follow_symlinks=True, append_version=True)
    web_app.router.add_get("/", lambda request: web.HTTPFound("/index.html"))
    web.run_app(web_app, port=8080)
    return 0


if __name__ == "__main__":
    rv = main()
    sys.exit(rv)
