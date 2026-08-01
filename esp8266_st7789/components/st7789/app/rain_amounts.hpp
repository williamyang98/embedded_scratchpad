#pragma once
#include <stdint.h>

// https://en.wikipedia.org/wiki/Rain#Intensity
enum class RainCategory: uint8_t {
    DRY = 0,
    LIGHT = 1,
    MODERATE = 2,
    HEAVY = 3,
    VIOLENT = 4,
};

constexpr uint8_t TOTAL_RAIN_CATEGORIES = 5;
static const char* const RAIN_DESCRIPTIONS[TOTAL_RAIN_CATEGORIES] = {
    "DRY",
    "LIGHT",
    "MODERATE",
    "HEAVY",
    "VIOLENT",
};

// rain_mm = rain_mm*10 to store 1 decimal point
static RainCategory get_rain_category(uint16_t rain_mm) {
    if (rain_mm == 0) {
        return RainCategory::DRY;
    } else if (rain_mm <= 25) {
        return RainCategory::LIGHT;
    } else if (rain_mm <= 76) {
        return RainCategory::MODERATE;
    } else if (rain_mm <= 500) {
        return RainCategory::HEAVY;
    } else {
        return RainCategory::VIOLENT;
    }
}

static const char* get_rain_description(RainCategory category) {
    const uint8_t index = static_cast<uint8_t>(category);
    if (index >= TOTAL_RAIN_CATEGORIES) return nullptr;
    return RAIN_DESCRIPTIONS[index];
}
