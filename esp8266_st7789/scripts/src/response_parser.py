import cobs
from frame import Frame
from abc import ABC, abstractmethod
import struct

# python receiver counterpart to ResponseHeader and ResponseSender in ../src/response.hpp
class ResponseHeader:
    ACKNOWLEDGE_COMMAND = 0x00
    RENDER_STATUS = 0x01
    LOG_MESSAGE = 0x02
    DEBUG_MESSAGE = 0x03
    DEBUG_FRAME = 0x04

class EmptyDecodedResponse(Exception):
    pass

class ResponseIncorrectLength(Exception):
    def __init__(self, header, expected_length, given_length):
        message = f"header={header:02X} has incorrect length expected={expected_length} but given={given_length}"
        super().__init__(message)
        self.header = header
        self.expected_length
        self.given_length

class UnhandledResponse(Exception):
    def __init__(self, header, data):
        message = f"Unhandled response with header={header:02X} with data body containing {len(data)-1} bytes"
        super().__init__(message)
        self.data = data
        self.header = header

class ResponseHandler(ABC):
    @abstractmethod
    def acknowledge_command(self, header: int, is_success: bool):
        pass

    @abstractmethod
    def render_status(self, is_busy: bool):
        pass

    @abstractmethod
    def log_message(self, message: str):
        pass

    @abstractmethod
    def debug_message(self, message: str):
        pass

    @abstractmethod
    def debug_frame(self, frame: Frame):
        pass

class ResponseParser:
    def __init__(self, handler: ResponseHandler):
        self.handler = handler

    def read_encoded_bytes(self, encoded_data: bytes):
        if len(encoded_data) == 0:
            return
        data = cobs.decode(encoded_data)
        if len(data) == 0:
            raise EmptyDecodedResponse

        header = data[0]

        def assert_length(expected_length):
            given_length = len(data)
            if given_length != expected_length:
                raise ResponseIncorrectLength(header, expected_length, given_length)

        if header == ResponseHeader.ACKNOWLEDGE_COMMAND:
            assert_length(3)
            ack_header = data[1]
            is_success = data[2] != 0
            self.handler.acknowledge_command(ack_header, is_success)
        elif header == ResponseHeader.RENDER_STATUS:
            assert_length(2)
            is_busy = data[1] != 0
            self.handler.render_status(is_busy)
        elif header == ResponseHeader.LOG_MESSAGE:
            message = data[1:]
            message = message.decode("utf-8")
            self.handler.log_message(message)
        elif header == ResponseHeader.DEBUG_MESSAGE:
            message = data[1:]
            message = message.decode("utf-8")
            self.handler.debug_message(message)
        elif header == ResponseHeader.DEBUG_FRAME:
            frame_data = data[1:]
            frame = Frame(frame_data)
            self.handler.debug_frame(frame)
        else:
            raise UnhandledResponse(header, data)

