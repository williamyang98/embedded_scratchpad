#pragma once
#include <stdint.h>
#include "../glyphs/icons.hpp"

struct MoonPhaseIcon {
    icons::small::Icon icon;
    bool is_horizontally_flipped;
    bool operator==(const MoonPhaseIcon& other) {
        return icon == other.icon && is_horizontally_flipped == other.is_horizontally_flipped;
    }
    bool operator!=(const MoonPhaseIcon& other) {
        return icon != other.icon || is_horizontally_flipped != other.is_horizontally_flipped;
    }
    const glyph::Glyph* get_glyph() const {
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

static const char* const MOON_PHASE_DESCRIPTIONS[TOTAL_MOON_PHASES] = {
    "NEW MOON",
    "WAXING CRESCENT",
    "FIRST QUARTER",
    "WAXING GIBBOUS",
    "FULL MOON",
    "WANING GIBBOUS",
    "THIRD QUARTER",
    "WANING CRESCENT",
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

static const char* get_moon_phase_description(MoonPhase phase) {
    const uint8_t index = static_cast<uint8_t>(phase);
    if (index >= TOTAL_MOON_PHASES) return nullptr;
    return MOON_PHASE_DESCRIPTIONS[index];
}
