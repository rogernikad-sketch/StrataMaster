#pragma once

#include <cmath>
#include <numbers>

namespace strata::dsp {

// Mathematical constants
inline constexpr float PI = std::numbers::pi_v<float>;
inline constexpr float TWO_PI = 2.0f * PI;
inline constexpr float HALF_PI = PI * 0.5f;
inline constexpr float SQRT_TWO = std::numbers::sqrt2_v<float>;
inline constexpr float INV_SQRT_TWO = 1.0f / SQRT_TWO;

// Audio engine thresholds
inline constexpr float SILENCE_THRESHOLD = 1.0e-5f; // ~ -100 dBFS
inline constexpr float MIN_FREQUENCY_HZ = 20.0f;
inline constexpr float MAX_FREQUENCY_HZ = 20000.0f;
inline constexpr float DEFAULT_SAMPLE_RATE = 44100.0f;

// Dynamics & limiting defaults
inline constexpr float MIN_DB = -96.0f;
inline constexpr float MAX_DB = 24.0f;
inline constexpr float TRUE_PEAK_CEILING_DEFAULT = -0.1f; // dBTP

} // namespace strata::dsp
