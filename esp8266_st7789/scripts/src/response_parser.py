from frame import Frame
from abc import ABC, abstractmethod
from enum import IntEnum
import struct
import logging

logger = logging.getLogger(__name__)

# python receiver counterpart to ResponseHeader and ResponseSender in ../src/response.hpp
class ResponseHeader:
    ACKNOWLEDGE_COMMAND = 0x00
    RENDER_STATUS = 0x01
    LOG_MESSAGE = 0x02
    DEBUG_MESSAGE = 0x03
    DEBUG_FRAME = 0x04
    # non-st7789 responses
    GET_DHT11 = 0xA0

class EmptyDecodedResponse(Exception):
    pass

class MinimumLengthError(Exception):
    def __init__(self, header, minimum, given):
        message = f"header {header:02X} expected minimum length of {minimum} but was given {given}"
        super().__init__(message)
        self.header = header
        self.minimum = minimum
        self.given = given

class ExactLengthError(Exception):
    def __init__(self, header, expected, given):
        message = f"header {header:02X} expected exact length of {expected} but was given {given}"
        super().__init__(message)
        self.header = header
        self.expected = expected
        self.given = given

class UnhandledResponse(Exception):
    def __init__(self, header, data):
        message = f"Unhandled response with header={header:02X} with data body containing {len(data)-1} bytes"
        super().__init__(message)
        self.data = data
        self.header = header

# components/dht11/include/dht11.hpp
class DHT11ReadStatus(IntEnum):
    OK = 0x00
    START_PULLDOWN_1_TIMEOUT = 0x01
    START_PULLUP_TIMEOUT = 0x01
    START_PULLDOWN_2_TIMEOUT = 0x02
    DATA_START_TO_TRANSMIT_TIMEOUT = 0x3
    DATA_PULLDOWN_TIMEOUT = 0x4
    DATA_PULLUP_TIMEOUT = 0x5
    DATA_PULLUP_TOO_SHORT = 0x6
    DATA_PULLUP_TOO_LONG = 0x7
    CHECKSUM_FAIL = 0x8

class DHT11Response:
    def __init__(self, temperature: int | None, humidity: int | None, error_code: int | None):
        self.is_success = error_code == None
        if self.is_success:
            assert temperature != None, "temperature field must be provided on success"
            assert humidity != None, "humidity field must be provided on success"
        self.temperature = temperature
        self.humidity = humidity
        if error_code != None:
            try:
                error_code = DHT11Status(error_code)
            except ValueError:
                logger.warning(f"Unhandled dht11 read status {error_code}")
        self.error_code = error_code

class ResponseHandler(ABC):
    @abstractmethod
    async def acknowledge_command(self, header: int, is_success: bool):
        pass

    @abstractmethod
    async def render_status(self, is_busy: bool):
        pass

    @abstractmethod
    async def log_message(self, message: str):
        pass

    @abstractmethod
    async def debug_message(self, message: str):
        pass

    @abstractmethod
    async def debug_frame(self, frame: Frame):
        pass

    @abstractmethod
    async def get_dht11(self, dht11: DHT11Response):
        pass

class ResponseParser:
    def __init__(self, handler: ResponseHandler):
        self.handler = handler

    async def read(self, data: bytearray):
        if len(data) == 0:
            raise EmptyDecodedResponse

        header = data[0]

        def assert_minimum_length(minimum):
            given = len(data)
            if given < minimum:
                raise MinimumLengthError(header, minimum, given)

        def assert_exact_length(expected):
            given = len(data)
            if given != expected:
                raise ExactLengthError(header, expected, given)

        if header == ResponseHeader.ACKNOWLEDGE_COMMAND:
            assert_exact_length(3)
            ack_header = data[1]
            is_success = data[2] != 0
            await self.handler.acknowledge_command(ack_header, is_success)
        elif header == ResponseHeader.RENDER_STATUS:
            assert_exact_length(2)
            is_busy = data[1] != 0
            await self.handler.render_status(is_busy)
        elif header == ResponseHeader.LOG_MESSAGE:
            message = data[1:]
            message = message.decode("utf-8")
            await self.handler.log_message(message)
        elif header == ResponseHeader.DEBUG_MESSAGE:
            message = data[1:]
            message = message.decode("utf-8")
            await self.handler.debug_message(message)
        elif header == ResponseHeader.DEBUG_FRAME:
            frame_data = data[1:]
            frame = Frame(frame_data)
            await self.handler.debug_frame(frame)
        elif header == ResponseHeader.GET_DHT11:
            assert_minimum_length(2)
            if len(data) == 2:
                error_code = data[1]
                dht11 = DHT11Response(None, None, error_code)
                await self.handler.get_dht11(dht11)
            elif len(data) == 3:
                humidity = data[1]
                temperature = data[2]
                dht11 = DHT11Response(temperature, humidity, None)
                await self.handler.get_dht11(dht11)
            else:
                raise Exception(f"Unhandled dht11 response with header={header:02X}, length={len(data)}")
        else:
            raise UnhandledResponse(header, data)

