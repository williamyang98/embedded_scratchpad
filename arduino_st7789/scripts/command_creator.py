import cobs
from enum import IntEnum
import functools
import inspect

# ../src/app/commands.hpp
class CommandHeader(IntEnum):
    TRIGGER_RENDER = 0x00
    # weather page
    SET_TEMPERATURE = 0x01
    SET_HUMIDITY = 0x02
    SET_TIME_24_HOUR = 0x03
    SET_WIND_KPH = 0x04
    SET_WEATHER_ICON = 0x05
    SET_LOCATION = 0x06
    SET_WEATHER_DESCRIPTION = 0x07
    SET_MOON_PHASE = 0x08
    # set page
    SET_SCREEN_BRIGHTNESS = 0xFE
    SET_PAGE = 0xFF

# ../src/app/weather_icons.hpp
class WeatherIcon(IntEnum):
    WINTER = 0
    LIGHTNING_STORM = 1
    HEAVY_RAIN = 2
    PARTLY_CLOUDY = 3
    SUNNY = 4

# ../src/app/moon_phases.hpp
class MoonPhase(IntEnum):
    NEW_MOON = 0
    WAXING_CRESCENT = 1
    FIRST_QUARTER = 2
    WAXING_GIBBOUS = 3
    FULL_MOON = 4
    WANING_GIBBOUS = 5
    THIRD_QUARTER = 6
    WANING_CRESCENT = 7

# ../src/app/app.hpp
class AppPage(IntEnum):
    LANDING_SCREEN = 0
    WEATHER_PAGE = 1

class CommandCreator:
    def trigger_render(self):
        return cobs.encode(bytearray([
            int(CommandHeader.TRIGGER_RENDER),
        ]))

    def set_page(self, page):
        assert isinstance(page, AppPage)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_PAGE),
            int(page),
        ]))

    def set_screen_brightness(self, brightness):
        assert isinstance(brightness, int)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_SCREEN_BRIGHTNESS),
            brightness & 0xFF,
        ]))

    def set_temperature(self, temperature):
        assert isinstance(temperature, int)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_TEMPERATURE),
            (temperature >> 8) & 0xFF,
            temperature & 0xFF,
        ]))

    def set_humidity(self, humidity):
        assert isinstance(humidity, int)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_HUMIDITY),
            (humidity >> 8) & 0xFF,
            humidity & 0xFF,
        ]))

    def set_24_hour_time(self, time_24_hour, is_show_24_hour, is_show_leading_zero):
        assert isinstance(time_24_hour, int)
        assert isinstance(is_show_24_hour, bool)
        assert isinstance(is_show_leading_zero, bool)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_TIME_24_HOUR),
            (time_24_hour >> 8) & 0xFF,
            time_24_hour & 0xFF,
            0xFF if is_show_24_hour else 0x00,
            0xFF if is_show_leading_zero else 0x00,
        ]))

    def set_wind_kph(self, wind_kph):
        assert isinstance(wind_kph, int)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_WIND_KPH),
            (wind_kph >> 8) & 0xFF,
            wind_kph & 0xFF,
        ]))

    def set_weather_icon(self, weather_icon: WeatherIcon):
        assert isinstance(weather_icon, WeatherIcon)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_WEATHER_ICON),
            int(weather_icon),
        ]))

    def set_location(self, location):
        assert isinstance(location, str)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_LOCATION),
            *location.encode(),
        ]))

    def set_weather_description(self, weather_description):
        assert isinstance(weather_description, str)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_WEATHER_DESCRIPTION),
            *weather_description.encode(),
        ]))

    def set_moon_phase(self, moon_phase: MoonPhase):
        assert isinstance(moon_phase, MoonPhase)
        return cobs.encode(bytearray([
            int(CommandHeader.SET_MOON_PHASE),
            int(moon_phase),
        ]))

def create_command_sender(cls):
    namespace = {}
    def __init__(self, writer=None):
        self.writer = writer
    namespace["__init__"] = __init__
    for name, method in inspect.getmembers(cls, inspect.isfunction):
        @functools.wraps(method)
        def wrapper(self, *args, __method=method, **kwargs):
            result = __method(self, *args, **kwargs)
            if self.writer == None:
                return None
            return self.writer(result)
        namespace[name] = wrapper
    return type("CommandSender", (cls,), namespace)

CommandSender = create_command_sender(CommandCreator)

