#pragma once

#include <cstddef>
#include <vector>
#include <atomic>
#include "BiquadFilter.h"

namespace strata::dsp {

/**
 * Maserati Analog Tone Processor.
 * Inspired by classic signature analog master-bus tone shaping units.
 * Features:
 * - BASS: Dual-stage low punch (60Hz resonant contour + 100Hz body)
 * - TREBLE: High-sheen air & clarity (10kHz - 16kHz Baxandall shelf + exciter sheen)
 * - SENSITIVITY: Dynamic input drive & harmonic sensitivity threshold
 */
class MaseratiProcessor {
public:
    MaseratiProcessor();
    ~MaseratiProcessor() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset() noexcept;

    void setBass(float bassDb) noexcept;
    void setTreble(float trebleDb) noexcept;
    void setSensitivity(float sensitivityDb) noexcept;
    void setEnabled(bool enabled) noexcept;

    [[nodiscard]] float getBass() const noexcept { return bassDb_; }
    [[nodiscard]] float getTreble() const noexcept { return trebleDb_; }
    [[nodiscard]] float getSensitivity() const noexcept { return sensitivityDb_; }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }
    [[nodiscard]] float getSensitivityActivity() const noexcept { return activityMeter_.load(std::memory_order_relaxed); }

    void processStereo(float* left, float* right, size_t numSamples) noexcept;

private:
    void updateFilters() noexcept;

    double sampleRate_{ 44100.0 };
    float bassDb_{ 0.0f };
    float trebleDb_{ 0.0f };
    float sensitivityDb_{ 0.0f };
    bool enabled_{ true };

    // Dual-channel tone shaping filters
    BiquadFilter bassFilterL_[2];   // 0: 60Hz punch bell, 1: 100Hz low shelf
    BiquadFilter bassFilterR_[2];

    BiquadFilter trebleFilterL_[2]; // 0: 3.5kHz presence bell, 1: 12kHz air shelf
    BiquadFilter trebleFilterR_[2];

    // Sensitivity dynamic envelope follower
    float envDetector_{ 0.0f };
    std::atomic<float> activityMeter_{ 0.0f };
};

} // namespace strata::dsp
