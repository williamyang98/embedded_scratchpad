# python counterpart to cobs_encode(...) in ../src/cobs.hpp
COBS_DELIMITER_BYTE = 0x00

def cobs_encode(src_buffer: bytearray):
    src_length_bytes = len(src_buffer)
    if src_length_bytes > 254:
        src_length_bytes = 254

    dest_buffer = bytearray(src_length_bytes+2)
    dest_i = 1
    previous_code_dest_i = 0
    curr_code_offset = 1
    for src_i in range(src_length_bytes):
        src_byte = src_buffer[src_i]
        if src_byte != COBS_DELIMITER_BYTE:
            dest_buffer[dest_i] = src_byte
        else:
            dest_buffer[previous_code_dest_i] = curr_code_offset
            previous_code_dest_i = dest_i
            curr_code_offset = 0
        dest_i += 1
        curr_code_offset += 1

    dest_buffer[previous_code_dest_i] = curr_code_offset
    dest_buffer[dest_i] = COBS_DELIMITER_BYTE
    return dest_buffer

# python counterpart to CommandHeader and CommandSender in ../src/commands.hpp
class CommandHeader:
    TRIGGER_RENDER = 0x00
    SET_TEMPERATURE = 0x01
    SET_HUMIDITY = 0x02
    SET_TIME_24_HOUR = 0x03
    SET_WIND_KPH = 0x04

class CommandCreator:
    def trigger_render(self):
        return cobs_encode(bytearray([
            CommandHeader.TRIGGER_RENDER
        ]))

    def set_temperature(self, temperature):
        return cobs_encode(bytearray([
            CommandHeader.SET_TEMPERATURE,
            (temperature >> 8) & 0xFF,
            temperature & 0xFF,
        ]))

    def set_humidity(self, humidity):
        return cobs_encode(bytearray([
            CommandHeader.SET_HUMIDITY,
            (humidity >> 8) & 0xFF,
            humidity & 0xFF,
        ]))

    def set_24_hour_time(self, time_24_hour, is_show_24_hour, is_show_leading_zero):
        return cobs_encode(bytearray([
            CommandHeader.SET_TIME_24_HOUR,
            (time_24_hour >> 8) & 0xFF,
            time_24_hour & 0xFF,
            0xFF if is_show_24_hour else 0x00,
            0xFF if is_show_leading_zero else 0x00,
        ]))

    def set_wind_kph(self, wind_kph):
        return cobs_encode(bytearray([
            CommandHeader.SET_WIND_KPH,
            (wind_kph >> 8) & 0xFF,
            wind_kph & 0xFF,
        ]))

class CommandSender:
    def __init__(self, writer):
        self.writer = writer
        self.creator = CommandCreator()

    def trigger_render(self):
        return self.write(self.creator.trigger_render())

    def set_temperature(self, temperature):
        return self.write(self.creator.set_temperature(temperature))

    def set_humidity(self, humidity):
        return self.write(self.creator.set_humidity(humidity))

    def set_24_hour_time(self, time_24_hour, is_show_24_hour, is_show_leading_zero):
        return self.write(self.creator.set_24_hour_time(time_24_hour, is_show_24_hour, is_show_leading_zero))

    def set_wind_kph(self, wind_kph):
        return self.write(self.creator.set_wind_kph(wind_kph))

    def write(self, data):
        return self.writer(data)


