from collections import namedtuple
from command_creator import WeatherIcon

WeatherCode = namedtuple("WeatherCode", ["description", "weather_icon"])

WMO_WEATHER_CODES = {
    0: WeatherCode("Sunny", WeatherIcon.SUNNY),
    1: WeatherCode("Mainly Sunny", WeatherIcon.SUNNY),
    2: WeatherCode("Partly Cloudy", WeatherIcon.PARTLY_CLOUDY),
    3: WeatherCode("Cloudy", WeatherIcon.PARTLY_CLOUDY),
    45: WeatherCode("Foggy", WeatherIcon.PARTLY_CLOUDY),
    48: WeatherCode("Rime Fog", WeatherIcon.PARTLY_CLOUDY),
    51: WeatherCode("Light Drizzle", WeatherIcon.HEAVY_RAIN),
    53: WeatherCode("Drizzle", WeatherIcon.HEAVY_RAIN),
    55: WeatherCode("Heavy Drizzle", WeatherIcon.HEAVY_RAIN),
    56: WeatherCode("Light Cold Drizzle", WeatherIcon.HEAVY_RAIN),
    57: WeatherCode("Cold Drizzle", WeatherIcon.HEAVY_RAIN),
    61: WeatherCode("Light Rain", WeatherIcon.HEAVY_RAIN),
    63: WeatherCode("Rain", WeatherIcon.HEAVY_RAIN),
    65: WeatherCode("Heavy Rain", WeatherIcon.HEAVY_RAIN),
    66: WeatherCode("Light Cold Rain", WeatherIcon.HEAVY_RAIN),
    67: WeatherCode("Cold Rain", WeatherIcon.HEAVY_RAIN),
    71: WeatherCode("Light Snow", WeatherIcon.WINTER),
    73: WeatherCode("Snow", WeatherIcon.WINTER),
    75: WeatherCode("Heavy Snow", WeatherIcon.WINTER),
    77: WeatherCode("Snow Grains", WeatherIcon.WINTER),
    80: WeatherCode("Light Showers", WeatherIcon.HEAVY_RAIN),
    81: WeatherCode("Showers", WeatherIcon.HEAVY_RAIN),
    82: WeatherCode("Heavy Showers", WeatherIcon.HEAVY_RAIN),
    85: WeatherCode("Light Snow Showers", WeatherIcon.WINTER),
    86: WeatherCode("Snow Showers", WeatherIcon.WINTER),
    95: WeatherCode("Thunderstorm", WeatherIcon.LIGHTNING_STORM),
    96: WeatherCode("Light Hail Thunderstorm", WeatherIcon.LIGHTNING_STORM),
    99: WeatherCode("Hail Thunderstorm", WeatherIcon.LIGHTNING_STORM),
}
