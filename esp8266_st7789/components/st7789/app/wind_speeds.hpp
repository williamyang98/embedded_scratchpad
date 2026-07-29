#pragma once
#include <stdint.h>

// https://en.wikipedia.org/wiki/Beaufort_scale#Modern_scale
enum class WindCategory: uint8_t {
    CALM = 0,
    LIGHT_AIR = 1,
    LIGHT_BREEZE = 2,
    GENTLE_BREEZE = 3,
    MODERATE_BREEZE = 4,
    FRESH_BREEZE = 5,
    STRONG_BREEZE = 6,
    NEAR_GALE = 7,
    GALE = 8,
    STRONG_GALE = 9,
    STORM = 10,
    VIOLENT_STORM = 11,
    HURRICANE = 12,
};

constexpr uint8_t TOTAL_WIND_CATEGORIES = 13;

static const char* const WIND_DESCRIPTIONS[TOTAL_WIND_CATEGORIES] = {
    "CALM",
    "DRAFTY",
    "DRAFTY",
    "BREEZE",
    "BREEZE",
    "WINDY",
    "WINDY",
    "GALE",
    "GALE",
    "GALE",
    "STORM",
    "STORM",
    "HURRICANE",
};

// wind_speed_kph = wind_kph*10 to store 1 decimal point
static WindCategory get_wind_speed_category(uint16_t wind_speed_kph) {
    if (wind_speed_kph < 10) {
        return WindCategory::CALM;
    } else if (wind_speed_kph <= 50) {
        return WindCategory::LIGHT_AIR;
    } else if (wind_speed_kph <= 110) {
        return WindCategory::LIGHT_BREEZE;
    } else if (wind_speed_kph <= 190) {
        return WindCategory::GENTLE_BREEZE;
    } else if (wind_speed_kph <= 280) {
        return WindCategory::MODERATE_BREEZE;
    } else if (wind_speed_kph <= 380) {
        return WindCategory::FRESH_BREEZE;
    } else if (wind_speed_kph <= 490) {
        return WindCategory::STRONG_BREEZE;
    } else if (wind_speed_kph <= 610) {
        return WindCategory::NEAR_GALE;
    } else if (wind_speed_kph <= 740) {
        return WindCategory::GALE;
    } else if (wind_speed_kph <= 880) {
        return WindCategory::STRONG_GALE;
    } else if (wind_speed_kph <= 1020) {
        return WindCategory::STORM;
    } else if (wind_speed_kph <= 1170) {
        return WindCategory::VIOLENT_STORM;
    } else {
        return WindCategory::HURRICANE;
    }
}

static const char* get_wind_speed_description(WindCategory category) {
    const uint8_t index = static_cast<uint8_t>(category);
    if (index >= TOTAL_WIND_CATEGORIES) return nullptr;
    return WIND_DESCRIPTIONS[index];
}
