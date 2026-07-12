#pragma once
#include <stdint.h>
#include "../hardware/pgmspace.h"

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

static const char WIND_DESCRIPTION_CALM[] PROGMEM = "CALM";
static const char WIND_DESCRIPTION_LIGHT_AIR[] PROGMEM = "DRAFTY";
static const char WIND_DESCRIPTION_LIGHT_BREEZE[] PROGMEM = "DRAFTY";
static const char WIND_DESCRIPTION_GENTLE_BREEZE[] PROGMEM = "BREEZE";
static const char WIND_DESCRIPTION_MODERATE_BREEZE[] PROGMEM = "BREEZE";
static const char WIND_DESCRIPTION_FRESH_BREEZE[] PROGMEM = "WINDY";
static const char WIND_DESCRIPTION_STRONG_BREEZE[] PROGMEM = "WINDY";
static const char WIND_DESCRIPTION_NEAR_GALE[] PROGMEM = "GALE";
static const char WIND_DESCRIPTION_GALE[] PROGMEM = "GALE";
static const char WIND_DESCRIPTION_STRONG_GALE[] PROGMEM = "GALE";
static const char WIND_DESCRIPTION_STORM[] PROGMEM = "STORM";
static const char WIND_DESCRIPTION_VIOLENT_STORM[] PROGMEM = "STORM";
static const char WIND_DESCRIPTION_HURRICANE[] PROGMEM = "HURRICANE";

static const char* const WIND_DESCRIPTIONS[TOTAL_WIND_CATEGORIES] PROGMEM = {
    WIND_DESCRIPTION_CALM,
    WIND_DESCRIPTION_LIGHT_AIR,
    WIND_DESCRIPTION_LIGHT_BREEZE,
    WIND_DESCRIPTION_GENTLE_BREEZE,
    WIND_DESCRIPTION_MODERATE_BREEZE,
    WIND_DESCRIPTION_FRESH_BREEZE,
    WIND_DESCRIPTION_STRONG_BREEZE,
    WIND_DESCRIPTION_NEAR_GALE,
    WIND_DESCRIPTION_GALE,
    WIND_DESCRIPTION_STRONG_GALE,
    WIND_DESCRIPTION_STORM,
    WIND_DESCRIPTION_VIOLENT_STORM,
    WIND_DESCRIPTION_HURRICANE,
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

static const FlashMemory<char>* get_wind_speed_description(WindCategory category) {
    const uint8_t index = static_cast<uint8_t>(category);
    if (index >= TOTAL_WIND_CATEGORIES) return nullptr;
    return reinterpret_cast<const FlashMemory<char>*>(pgm_read_ptr(WIND_DESCRIPTIONS + index));
}
