#pragma once

#include "Constants.h"
#include <algorithm>
#include <cmath>

namespace strata::dsp {

/** Converts linear amplitude gain to decibels. */
[[nodiscard]] inline float gainToDecibels(float gain, float minusInfinityDb = MIN_DB) noexcept {
    if (gain <= 0.0f) {
        return minusInfinityDb;
    }
    const float db = 20.0f * std::log10(gain);
    return (db < minusInfinityDb) ? minusInfinityDb : db;
}

/** Converts decibels to linear amplitude gain. */
[[nodiscard]] inline float decibelsToGain(float decibels) noexcept {
    if (decibels <= MIN_DB) {
        return 0.0f;
    }
    return std::pow(10.0f, decibels * 0.05f);
}

} // namespace strata::dsp
