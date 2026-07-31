from abc import ABC, abstractmethod
import cobs
from command_creator import CommandSender
from response_parser import ResponseParser, ResponseHandler
import threading
from typing_extensions import override
import logging
import asyncio
import subprocess
import serial
from aiohttp import WSMsgType, ClientSession
import os
import sys
import argparse

logger = logging.getLogger(__name__)

class Device(ABC):
    def __init__(self):
        pass

    @abstractmethod
    async def write(self, data: bytearray):
        pass

    @abstractmethod
    async def read(self) -> bytearray:
        pass

    @abstractmethod
    async def close(self, force=False):
        pass

class ProcessDevice(Device):
    def __init__(self, event_loop, exec_filepath, exec_args):
        super().__init__()
        self.exec_filepath = exec_filepath
        self.exec_args = exec_args
        self.exec_filename = os.path.basename(exec_args[0])
        self.logger = logger.getChild(f"process({self.exec_filename})")

        self.process = subprocess.Popen(
            [self.exec_filepath, *self.exec_args],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self.event_loop = event_loop
        self.read_queue = asyncio.Queue(maxsize=16)
        self.write_queue = asyncio.Queue(maxsize=16)
        self.read_thread = None
        self.write_thread = None
        self._start_threads()

    @override
    async def write(self, data: bytearray):
        encoded_data = cobs.encode(data)
        await self.write_queue.put(encoded_data)

    @override
    async def read(self) -> bytearray:
        encoded_data = await self.read_queue.get()
        decoded_data = cobs.decode(encoded_data)
        return decoded_data

    def _start_threads(self):
        self.read_thread = threading.Thread(target=self._read_data)
        self.write_thread = threading.Thread(target=self._write_data)
        self.read_thread.start()
        self.write_thread.start()

    def _read_data(self):
        def read_frame():
            buffer = []
            while True:
                try:
                    res = self.process.stdout.read(1)
                except Exception as ex:
                    self.logger.info(f"Error while reading from stdout: {ex}")
                    return None
                if res == None:
                    self.logger.info("Exiting since stdout closed")
                    return None
                if len(res) == 0:
                    self.logger.info(f"Exiting since stdout did not provide any data")
                    return None
                c = res[0]
                buffer.append(c)
                if c == cobs.DELIMITER_BYTE:
                    return buffer

        while True:
            buffer = read_frame()
            if buffer == None:
                break
            buffer = bytearray(buffer)
            try:
                future = asyncio.run_coroutine_threadsafe(self.read_queue.put(buffer), self.event_loop)
                future.result()
            except Exception as ex:
                self.logger.error(f"Error while sending buffer to read_queue: {ex}")
                break

        self.read_queue.shutdown()

    def _write_data(self):
        while True:
            try:
                future = asyncio.run_coroutine_threadsafe(self.write_queue.get(), self.event_loop)
                buffer = future.result(timeout=1000)
                if buffer is None:
                    break
            except Exception as ex:
                self.logger.error(f"Error while receiving buffer from write_queue: {ex}")
                break

            try:
                self.process.stdin.write(buffer)
                self.process.stdin.flush()
            except Exception as ex:
                self.logger.error(f"Error while writing to stdin: {ex}")
                break
        self.write_queue.shutdown()

    @override
    def close(self, force=False):
        self.process.stdin.close()
        if force:
            self.process.terminate()
        self.read_queue.shutdown(immediate=force)
        self.write_queue.shutdown(immediate=force)
        self.read_thread.join()
        self.write_thread.join()

class SerialDevice(Device):
    def __init__(self, event_loop, port_name: str, baud_rate: int, is_reset: bool):
        super().__init__()
        ser = serial.Serial()
        ser.port = port_name
        ser.baudrate = baud_rate
        ser.dtr = is_reset
        ser.open()

        self.logger = logger.getChild(f"serial({port_name})")
        self.serial = ser
        self.event_loop = event_loop
        self.read_thread = None
        self.write_thread = None
        self.read_queue = asyncio.Queue(maxsize=16)
        self.write_queue = asyncio.Queue(maxsize=16)
        self._start_threads()

    @override
    async def write(self, data: bytearray):
        encoded_data = cobs.encode(data)
        await self.write_queue.put(encoded_data)

    @override
    async def read(self) -> bytearray:
        encoded_data = await self.read_queue.get()
        decoded_data = cobs.decode(encoded_data)
        return decoded_data

    def _start_threads(self):
        self.read_thread = threading.Thread(target=self._read_data)
        self.write_thread = threading.Thread(target=self._write_data)
        self.read_thread.start()
        self.write_thread.start()

    def _read_data(self):
        while True:
            try:
                buffer = self.serial.read_until(expected=bytes([cobs.DELIMITER_BYTE]))
            except Exception as ex:
                self.logger.info(f"Error while reading from port: {ex}")
                break
            buffer = bytearray(buffer)
            try:
                future = asyncio.run_coroutine_threadsafe(self.read_queue.put(buffer), self.event_loop)
                future.result()
            except Exception as ex:
                self.logger.error(f"Error while sending buffer to read_queue: {ex}")
                break

        self.read_queue.shutdown()

    def _write_data(self):
        while True:
            try:
                future = asyncio.run_coroutine_threadsafe(self.write_queue.get(), self.event_loop)
                buffer = future.result(timeout=1000)
            except Exception as ex:
                self.logger.error(f"Error while receiving buffer from write_queue: {ex}")
                break

            try:
                self.serial.write(buffer)
            except Exception as ex:
                self.logger.error(f"Error while writing to port: {ex}")
                break
        self.write_queue.shutdown()

    @override
    def close(self, force=False):
        self.serial.close()
        self.read_queue.shutdown(immediate=force)
        self.write_queue.shutdown(immediate=force)
        self.read_thread.join()
        self.write_thread.join()

class WebsocketDevice(Device):
    def __init__(self, client: ClientSession, url: str, heartbeat: float, timeout: float):
        self.logger = logger.getChild(f"websocket({url})")
        self.url = url
        self.heartbeat = heartbeat
        self.timeout = timeout
        self.client = client
        self.websocket = None

    async def open(self):
        self.websocket = await self.client.ws_connect(self.url, heartbeat=self.heartbeat, timeout=self.timeout)

    @override
    async def write(self, data: bytearray):
        self._assert_open()
        await self.websocket.send_bytes(data)

    @override
    async def read(self) -> bytearray:
        self._assert_open()
        while True:
            message = await self.websocket.receive()
            if message.type == WSMsgType.TEXT:
                self.logger.info(f"Got message: {message.data}")
            elif message.type == WSMsgType.BINARY:
                return bytearray(message.data)
            elif message.type == WSMsgType.PING:
                self.logger.info("Got ping")
            elif message.type == WSMsgType.PONG:
                self.logger.info("Got pong")
            elif message.type == WSMsgType.CLOSE:
                self.logger.info("Got close message")
            elif message.type == WSMsgType.CLOSE_FRAME:
                raise Exception(f"Websocket is closed at url={self.url}")
            else:
                self.logger.warning(f"Got unhandled websocket with type: {message.type}")

    @override
    async def close(self):
        await self.websocket.close()

    def _assert_open(self):
        hint_message = "Did you forget to call await open()?"
        assert self.client != None, f"Client session has not been opened yet. {hint_message}"
        assert self.websocket != None, f"Websocket has not been opened yet for reading. {hint_message}"
        assert not self.websocket.closed, "Websocket has been closed"
        assert not self.client.closed, "Client session has been closed"

def add_device_argument_subparsers(subparsers: argparse._SubParsersAction):
    assert isinstance(subparsers, argparse._SubParsersAction), "subparsers should be an instance of argparse._SubParsersAction created from parser.add_subparsers(...)"

    # process
    parser_process = subparsers.add_parser("process", help="Launch process")
    def get_default_exec():
        tests_dir = os.getenv("BUILD_TESTS_DIR", "./build/tests")
        filename = "st7789"
        if sys.platform == "win32":
            filename += ".exe"
        return os.path.join(tests_dir, filename)
    parser_process.add_argument("--exec", default=get_default_exec(), help="Process executable path")
    parser_process.add_argument("--args", nargs="*", default=[], help="Process executable arguments")

    # serial
    parser_serial = subparsers.add_parser("serial", help="Connect to serial COM port")
    parser_serial.add_argument("--port", default=None, type=str, help="COM port to connect to")
    parser_serial.add_argument("--baud", default=9600, type=int, help="Rate to communicate with device")
    parser_serial.add_argument("--list", action="store_true", help="List all COM ports connected to computer")
    parser_serial.add_argument("--reset", action="store_true", help="Reset on connection")

    # websocket
    parser_websocket = subparsers.add_parser("websocket", help="Connect to websocket at a url")
    parser_websocket.add_argument("--url", default="http://localhost:8080/api/v1/websocket", type=str, help="Url of websocket")
    parser_websocket.add_argument("--heartbeat", default=10, type=float, help="Period of websocket heartbeat to ping device")
    parser_websocket.add_argument("--timeout", default=10, type=float, help="Duration before websocket times out")

class DeviceFactory(ABC):
    def __init__(self):
        pass

    @abstractmethod
    async def create_device(self) -> Device:
        pass

class ProcessDeviceFactory(DeviceFactory):
    def __init__(self, *args, **kwargs):
        super().__init__()
        self.args = args
        self.kwargs = kwargs

    @override
    async def create_device(self) -> ProcessDevice:
        event_loop = asyncio.get_running_loop()
        return ProcessDevice(event_loop, *self.args, **self.kwargs)

class SerialDeviceFactory(DeviceFactory):
    def __init__(self, *args, **kwargs):
        super().__init__()
        self.args = args
        self.kwargs = kwargs

    @override
    async def create_device(self) -> SerialDevice:
        event_loop = asyncio.get_running_loop()
        return SerialDevice(event_loop, *self.args, **self.kwargs)

class WebsocketDeviceFactory(DeviceFactory):
    def __init__(self, *args, **kwargs):
        super().__init__()
        self.client = None
        self.args = args
        self.kwargs = kwargs

    @override
    async def create_device(self) -> WebsocketDevice:
        if self.client == None:
            self.client = ClientSession()
        device = WebsocketDevice(self.client, *self.args, **self.kwargs)
        await device.open()
        return device

def get_device_factory_from_args(mode, args):
    # counterpart to add_device_argument_subparsers(subparsers)
    choices = ["process", "serial", "websocket"]
    assert mode in choices, f"{mode} is not a valid device type: [{choices.join(',')}]"

    def as_exit_code(code):
        return {
            "type": "exit_code",
            "exit_code": code,
        }

    def as_device_factory(factory):
        return {
            "type": "device_factory",
            "device_factory": factory,
        }

    if mode == "process":
        filepath = os.path.abspath(args.exec)
        if not os.path.isfile(filepath):
            logger.error(f"'{filepath}' is not a valid executable")
            return as_exit_code(1)
        return as_device_factory(ProcessDeviceFactory(filepath, args.exec))

    if mode == "serial":
        if args.list:
            import serial.tools.list_ports
            ports = serial.tools.list_ports.comports()
            if len(ports) == 0:
                print("No ports are available to list", file=sys.stderr)
                return as_exit_code(1)
            print(f"Listing {len(ports)} ports")
            for port in ports:
                print(f"> {port.device}")
            return as_exit_code(0)

        port = args.port
        if port == None:
            import serial.tools.list_ports
            ports = serial.tools.list_ports.comports()
            if len(ports) == 0:
                print("No ports available to choose by default", file=sys.stderr)
                return as_exit_code(1)
            port = ports[0]
            print(f"Choosing first port '{port.device}' by default")
            port = port.device

        if args.reset:
            print(f"Resetting COM port {port} on connection")

        return as_device_factory(SerialDeviceFactory(port, args.baud, args.reset))

    if mode == "websocket":
        return as_device_factory(WebsocketDeviceFactory(args.url, args.heartbeat, args.timeout))

