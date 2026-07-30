from abc import ABC, abstractmethod
import cobs
from command_creator import CommandSender
from response_parser import ResponseParser, ResponseHandler
import threading
from typing_extensions import override
import logging
import asyncio

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
    def __init__(self, process, event_loop):
        super().__init__()
        self.process = process
        self.read_thread = None
        self.write_thread = None
        self.read_queue = asyncio.Queue(maxsize=16)
        self.write_queue = asyncio.Queue(maxsize=16)
        self.event_loop = event_loop
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
                    logger.info(f"Error while reading from  process stdout: {ex}")
                    return None
                if res == None:
                    logger.info("Exiting since process stdout closed")
                    return None
                if len(res) == 0:
                    logger.info(f"Exiting since process stdout did not provide any data")
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
                logger.error(f"Error while writing read buffer to process async read_queue: {ex}")
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
                logger.error(f"Error while reading buffer from process async write_queue: {ex}")
                break

            try:
                self.process.stdin.write(buffer)
                self.process.stdin.flush()
            except Exception as ex:
                logger.error(f"Error while writing to process stdin: {ex}")
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
    def __init__(self, serial, event_loop):
        super().__init__()
        self.serial = serial
        self.read_thread = None
        self.write_thread = None
        self.read_queue = asyncio.Queue(maxsize=16)
        self.write_queue = asyncio.Queue(maxsize=16)
        self.event_loop = event_loop
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
                logger.info(f"Error while reading from serial port: {ex}")
                break
            buffer = bytearray(buffer)
            try:
                future = asyncio.run_coroutine_threadsafe(self.read_queue.put(buffer), self.event_loop)
                future.result()
            except Exception as ex:
                logger.error(f"Error while writing buffer to serial port async read_queue: {ex}")
                break

        self.read_queue.shutdown()

    def _write_data(self):
        while True:
            try:
                future = asyncio.run_coroutine_threadsafe(self.write_queue.get(), self.event_loop)
                buffer = future.result(timeout=1000)
            except Exception as ex:
                logger.error(f"Error while reading buffer from serial port async write_queue: {ex}")
                break

            try:
                self.serial.write(buffer)
            except Exception as ex:
                logger.error(f"Error while writing to serial port: {ex}")
                break
        self.write_queue.shutdown()

    @override
    def close(self, force=False):
        self.serial.close()
        self.read_queue.shutdown(immediate=force)
        self.write_queue.shutdown(immediate=force)
        self.read_thread.join()
        self.write_thread.join()
