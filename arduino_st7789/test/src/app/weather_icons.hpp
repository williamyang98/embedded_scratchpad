#pragma once

#include "../glyphs/icons.hpp"
#include "../hardware/pgmspace.h"

constexpr uint8_t TOTAL_WEATHER_ICONS = 5;
constexpr uint8_t WEATHER_ICON_MAX_HEIGHT = icons::large::MAX_HEIGHT;

enum class WeatherIcon: uint8_t {
    WINTER = 0,
    LIGHTNING_STORM = 1,
    HEAVY_RAIN = 2,
    PARTLY_CLOUDY = 3,
    SUNNY = 4,
};

static const icons::large::Icon WEATHER_ICONS[TOTAL_WEATHER_ICONS] = {
    icons::large::Icon::WINTER,
    icons::large::Icon::LIGHTNING_STORM,
    icons::large::Icon::HEAVY_RAIN,
    icons::large::Icon::PARTLY_CLOUDY,
    icons::large::Icon::SUNNY,
};

static const FlashMemory<glyph::Glyph>* get_weather_icon(WeatherIcon icon) {
    const uint8_t index = static_cast<uint8_t>(icon);
    if (index >= TOTAL_WEATHER_ICONS) return nullptr;
    const auto large_icon = WEATHER_ICONS[index];
    return icons::large::get_icon(large_icon);
}
