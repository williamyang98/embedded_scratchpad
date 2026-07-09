from abc import ABC, abstractmethod
import cobs
from command_creator import CommandSender
from response_parser import ResponseParser, ResponseHandler
import threading
from typing import override

class Device(ABC):
    def __init__(self, response_handler: ResponseHandler):
        self.response_handler = response_handler
        self.command_sender = CommandSender(self.write_data)
        self.response_parser = ResponseParser(self.response_handler)

    def get_command_sender(self):
        return self.command_sender

    @abstractmethod
    def write_data(self, data):
        pass

    @abstractmethod
    def wait(self, force=False):
        pass

class ProcessDevice(Device):
    def __init__(self, process, response_handler: ResponseHandler):
        super().__init__(response_handler)
        self.process = process
        self.read_thread = None
        self._start_threads()

    @override 
    def write_data(self, data):
        self.process.stdin.write(data)
        self.process.stdin.flush()

    def _start_threads(self):
        self.read_thread = threading.Thread(target=self._read_data)
        self.read_thread.start()

    def _read_data(self):
        while True:
            buffer = []
            while True:
                try:
                    res = self.process.stdout.read(1)
                except Exception as ex:
                    print(f"Exiting reader thread: {ex}")
                    return
                if res == None:
                    print("Exiting since stdout closed in reader thread")
                    return
                if len(res) == 0:
                    print(f"Exiting since stdout did not receive a byte")
                    return
                c = res[0]
                buffer.append(c)
                if c == cobs.DELIMITER_BYTE:
                    break
            buffer = bytearray(buffer)
            try:
                self.response_parser.read_encoded_bytes(buffer)
            except Exception as ex:
                print(f"Error while reading serial: {ex}")

    @override
    def wait(self, force=False):
        self.process.stdin.close()
        if force:
            self.process.terminate()
        self.read_thread.join()

class SerialDevice(Device):
    def __init__(self, serial, response_handler: ResponseHandler):
        super().__init__(response_handler)
        self.serial = serial
        self.read_thread = None
        self._start_threads()

    @override
    def write_data(self, data):
        self.serial.write(data)

    def _start_threads(self):
        self.read_thread = threading.Thread(target=self._read_data)
        self.read_thread.start()

    def _read_data(self):
        while True:
            try:
                data = self.serial.read_until(expected=bytes([cobs.DELIMITER_BYTE]))
            except Exception as ex:
                print(f"Exiting reader thread: {ex}")
                return
            try:
                self.response_parser.read_encoded_bytes(data)
            except Exception as ex:
                print(f"Error while reading serial: {ex}")

    @override
    def wait(self, force=False):
        self.serial.close()
        self.read_thread.join()
