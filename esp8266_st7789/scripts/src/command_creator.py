from enum import IntEnum
import functools
import inspect

# transmitter for components/st7789/app/commands.hpp
# websocket_on_binary_frame @ main/websocket_handler.cpp
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
    SET_RAIN_MM = 0x09
    # set page
    SET_SCREEN_BRIGHTNESS = 0xFE
    SET_PAGE = 0xFF
    # non st7789 commands
    GET_DHT11 = 0xA0

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

def to_fixed_point(value: float | int):
    # scale to fixed point with 1 decimal place
    value = value*10;
    value = int(round(value));
    return value

class CommandCreator:
    def trigger_render(self):
        return bytearray([
            int(CommandHeader.TRIGGER_RENDER),
        ])

    def set_page(self, page):
        assert isinstance(page, AppPage)
        return bytearray([
            int(CommandHeader.SET_PAGE),
            int(page),
        ])

    def set_screen_brightness(self, brightness):
        assert isinstance(brightness, int)
        return bytearray([
            int(CommandHeader.SET_SCREEN_BRIGHTNESS),
            brightness & 0xFF,
        ])

    def set_temperature(self, temperature):
        assert isinstance(temperature, int | float)
        temperature = to_fixed_point(temperature)
        return bytearray([
            int(CommandHeader.SET_TEMPERATURE),
            (temperature >> 8) & 0xFF,
            temperature & 0xFF,
        ])

    def set_humidity(self, humidity):
        assert isinstance(humidity, int | float)
        humidity = to_fixed_point(humidity)
        return bytearray([
            int(CommandHeader.SET_HUMIDITY),
            (humidity >> 8) & 0xFF,
            humidity & 0xFF,
        ])

    def set_24_hour_time(self, time_24_hour, is_show_24_hour, is_show_leading_zero):
        assert isinstance(time_24_hour, int)
        assert isinstance(is_show_24_hour, bool)
        assert isinstance(is_show_leading_zero, bool)
        return bytearray([
            int(CommandHeader.SET_TIME_24_HOUR),
            (time_24_hour >> 8) & 0xFF,
            time_24_hour & 0xFF,
            0xFF if is_show_24_hour else 0x00,
            0xFF if is_show_leading_zero else 0x00,
        ])

    def set_rain_mm(self, rain_mm):
        assert isinstance(rain_mm, int | float)
        rain_mm = to_fixed_point(rain_mm)
        return bytearray([
            int(CommandHeader.SET_RAIN_MM),
            (rain_mm >> 8) & 0xFF,
            rain_mm & 0xFF,
        ])

    def set_wind_kph(self, wind_kph):
        assert isinstance(wind_kph, int | float)
        wind_kph = to_fixed_point(wind_kph)
        return bytearray([
            int(CommandHeader.SET_WIND_KPH),
            (wind_kph >> 8) & 0xFF,
            wind_kph & 0xFF,
        ])

    def set_weather_icon(self, weather_icon: WeatherIcon):
        assert isinstance(weather_icon, WeatherIcon)
        return bytearray([
            int(CommandHeader.SET_WEATHER_ICON),
            int(weather_icon),
        ])

    def set_location(self, location):
        assert isinstance(location, str)
        return bytearray([
            int(CommandHeader.SET_LOCATION),
            *location.encode(),
        ])

    def set_weather_description(self, weather_description):
        assert isinstance(weather_description, str)
        return bytearray([
            int(CommandHeader.SET_WEATHER_DESCRIPTION),
            *weather_description.encode(),
        ])

    def set_moon_phase(self, moon_phase: MoonPhase):
        assert isinstance(moon_phase, MoonPhase)
        return bytearray([
            int(CommandHeader.SET_MOON_PHASE),
            int(moon_phase),
        ])

    def get_dht11(self):
        return bytearray([
            int(CommandHeader.GET_DHT11),
        ])

def create_command_sender(cls):
    namespace = {}
    def __init__(self, writer=None):
        self.writer = writer
    namespace["__init__"] = __init__
    for name, method in inspect.getmembers(cls, inspect.isfunction):
        @functools.wraps(method)
        async def wrapper(self, *args, __method=method, **kwargs):
            result = __method(self, *args, **kwargs)
            if self.writer == None:
                return None
            if inspect.iscoroutinefunction(self.writer):
                return await self.writer(result)
            return self.writer(result)
        namespace[name] = wrapper
    return type("CommandSender", (cls,), namespace)

CommandSender = create_command_sender(CommandCreator)

