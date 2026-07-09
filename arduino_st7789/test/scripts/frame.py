import struct
import logging

logger = logging.getLogger(__name__)

class FrameBodyTooShort(Exception):
    def __init__(self, minimum_length, data, part):
        given_length = len(data)
        message = f"Frame body too short to contain '{part}' expected minimum length of {minimum_length} bytes but got {given_length} bytes"
        super().__init__(message)
        self.minimum_length = minimum_length
        self.given_length = given_length
        self.data = data
        self.part = part

class Frame:
    def __init__(self, data):
        header_format = "<HHHHHHHHBBI"
        header_size = struct.calcsize(header_format)
        if len(data) < header_size:
            raise FrameBodyTooShort(header_size, data, "HEADER")

        header = struct.unpack(header_format, data[:header_size])
        x_start, x_end, y_start, y_end = header[:4]
        x_cursor, y_cursor = header[4:6]
        width, height = header[6:8]
        brightness, hardware_reset = header[8:10]
        label_length = header[10]

        data = data[header_size:]
        if len(data) < label_length:
            raise FrameBodyTooShort(label_length, data, "LABEL")
        label = data[:label_length]
        label = label.decode("utf-8")

        total_pixels = width*height
        rgb565_size = 2
        total_pixel_bytes = total_pixels*rgb565_size
        data = data[label_length:]
        if len(data) < total_pixel_bytes:
            raise FrameBodyTooShort(total_pixel_bytes, data, "PIXEL_DATA")
        pixel_data = data[:total_pixel_bytes]
        pixel_data = memoryview(pixel_data).cast("H")

        data = data[total_pixel_bytes:]
        if len(data) != 0:
            logger.warning(f"Got {len(data)} extra unhandled bytes in frame body")

        self.x_start = x_start
        self.x_end = x_end
        self.y_start = y_start
        self.y_end = y_end
        self.x_cursor = x_cursor
        self.y_cursor = y_cursor
        self.width = width
        self.height = height
        self.brightness = brightness
        self.hardware_reset = hardware_reset
        self.label = label
        self.pixel_data = pixel_data
