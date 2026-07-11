#pragma once
#include "../scripts/glyphs/icons.hpp"
#include "./pgmspace.h"

struct MoonPhaseIcon {
    icons::small::Icon icon;
    bool is_horizontally_flipped;
    bool operator==(const MoonPhaseIcon& other) {
        return icon == other.icon && is_horizontally_flipped == other.is_horizontally_flipped;
    }
    bool operator!=(const MoonPhaseIcon& other) {
        return icon != other.icon || is_horizontally_flipped != other.is_horizontally_flipped;
    }
    const FlashMemory<glyph::Glyph>* get_glyph() const {
        return icons::small::get_icon(icon);
    }
};

constexpr uint8_t TOTAL_MOON_PHASES = 8;

static const MoonPhaseIcon MOON_PHASE_ICONS[TOTAL_MOON_PHASES] = {
    { icons::small::Icon::MOON_NEW, false },             // new_moon
    { icons::small::Icon::MOON_WAXING_CRESCENT, false }, // waxing_crescent
    { icons::small::Icon::MOON_THIRD_QUARTER, true },    // first_quarter
    { icons::small::Icon::MOON_WANING_GIBBOUS, true },   // waxing_gibbous
    { icons::small::Icon::MOON_FULL, false },            // full_moon
    { icons::small::Icon::MOON_WANING_GIBBOUS, false },  // waning_gibbous
    { icons::small::Icon::MOON_THIRD_QUARTER, false },   // third_quarter
    { icons::small::Icon::MOON_WAXING_CRESCENT, true },  // waning_crescent
};

const char MOON_PHASE_DESCRIPTION_NEW_MOON[] PROGMEM = "NEW MOON";
const char MOON_PHASE_DESCRIPTION_WAXING_CRESCENT[] PROGMEM = "WAXING CRESCENT";
const char MOON_PHASE_DESCRIPTION_FIRST_QUARTER[] PROGMEM = "FIRST QUARTER";
const char MOON_PHASE_DESCRIPTION_WAXING_GIBBOUS[] PROGMEM = "WAXING GIBBOUS";
const char MOON_PHASE_DESCRIPTION_FULL_MOON[] PROGMEM = "FULL MOON";
const char MOON_PHASE_DESCRIPTION_WANING_GIBBOUS[] PROGMEM = "WANING GIBBOUS";
const char MOON_PHASE_DESCRIPTION_THIRD_QUARTER[] PROGMEM = "THIRD QUARTER";
const char MOON_PHASE_DESCRIPTION_WANING_CRESCENT[] PROGMEM = "WANING CRESCENT";

static const char* const MOON_PHASE_DESCRIPTIONS[TOTAL_MOON_PHASES] PROGMEM = {
    MOON_PHASE_DESCRIPTION_NEW_MOON,
    MOON_PHASE_DESCRIPTION_WAXING_CRESCENT,
    MOON_PHASE_DESCRIPTION_FIRST_QUARTER,
    MOON_PHASE_DESCRIPTION_WAXING_GIBBOUS,
    MOON_PHASE_DESCRIPTION_FULL_MOON,
    MOON_PHASE_DESCRIPTION_WANING_GIBBOUS,
    MOON_PHASE_DESCRIPTION_THIRD_QUARTER,
    MOON_PHASE_DESCRIPTION_WANING_CRESCENT,
};

enum class MoonPhase: uint8_t {
    NEW_MOON = 0,
    WAXING_CRESCENT = 1,
    FIRST_QUARTER = 2,
    WAXING_GIBBOUS = 3,
    FULL_MOON = 4,
    WANING_GIBBOUS = 5,
    THIRD_QUARTER = 6,
    WANING_CRESCENT = 7,
};

static const MoonPhaseIcon* get_moon_phase_icon(MoonPhase phase) {
    const uint8_t index = static_cast<uint8_t>(phase);
    if (index >= TOTAL_MOON_PHASES) return nullptr;
    return &MOON_PHASE_ICONS[index];
}

static const FlashMemory<char>* get_moon_phase_description(MoonPhase phase) {
    const uint8_t index = static_cast<uint8_t>(phase);
    if (index >= TOTAL_MOON_PHASES) return nullptr;
    return reinterpret_cast<const FlashMemory<char>*>(pgm_read_ptr(MOON_PHASE_DESCRIPTIONS + index));
}
