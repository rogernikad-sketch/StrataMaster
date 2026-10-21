#pragma once

#include "BiquadFilter.h"
#include "common/Constants.h"
#include <array>
#include <atomic>
#include <cstddef>

namespace strata::dsp {

inline constexpr size_t NUM_EQ_BANDS = 8;

struct DynamicBandSettings {
    bool enabled{true};
    FilterType type{FilterType::Bell};
    float frequencyHz{1000.0f};
    float gainDb{0.0f};
    float q{1.0f};

    // Dynamic processing parameters
    bool dynamicEnabled{false};
    float thresholdDb{-20.0f};
    float dynamicRangeDb{-6.0f}; // Negative for compression/cut, positive for expansion/boost
    float attackMs{10.0f};
    float releaseMs{100.0f};
};

/**
 * Single Dynamic Equalizer Band for a single audio channel.
 */
class DynamicBand final {
public:
    DynamicBand() noexcept = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void updateSettings(const DynamicBandSettings& settings, bool snapImmediately = false) noexcept;

    /** Processes audio sample through sidechain detector and dynamic biquad filter */
    float processSample(float input) noexcept;

    /** Processes block of audio */
    void processBlock(float* buffer, size_t numSamples) noexcept;

    [[nodiscard]] float getGainReductionDb() const noexcept {
        return gainReductionDb_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] double getMagnitudeResponseDb(double frequencyHz) const noexcept {
        if (!settings_.enabled) {
            return 0.0;
        }
        return audioFilter_.getMagnitudeResponseDb(frequencyHz);
    }

private:
    void updateEnvelopeBallistics() noexcept;

    double sampleRate_{44100.0};
    DynamicBandSettings settings_;

    // Main audio filter
    BiquadFilter audioFilter_;

    // Sidechain bandpass filter for isolating trigger frequency
    BiquadFilter sidechainFilter_;

    // Envelope detector
    float envelopeLevel_{0.0f};
    float attackCoeff_{0.0f};
    float releaseCoeff_{0.0f};

    // Current dynamic gain offset applied
    float dynamicOffsetDb_{0.0f};
    std::atomic<float> gainReductionDb_{0.0f};
};

/**
 * 8-Band Dynamic Equalizer supporting Stereo & Mid/Side processing.
 */
class DynamicEqualizer final {
public:
    DynamicEqualizer() noexcept = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setBandSettings(size_t bandIndex, const DynamicBandSettings& settings, bool snapImmediately = false) noexcept;
    [[nodiscard]] const DynamicBandSettings& getBandSettings(size_t bandIndex) const noexcept;

    /** Processes stereo block in-place (L/R) */
    void processStereo(float* left, float* right, size_t numSamples) noexcept;

    /** Gets gain reduction in dB for a band (channel 0 = L / Mid, channel 1 = R / Side) */
    [[nodiscard]] float getGainReductionDb(size_t bandIndex, size_t channel = 0) const noexcept;

    /** Calculates composite magnitude response across all enabled bands for UI plotting */
    [[nodiscard]] double getCompositeMagnitudeResponseDb(double frequencyHz) const noexcept;

private:
    double sampleRate_{44100.0};
    std::array<DynamicBandSettings, NUM_EQ_BANDS> bandSettings_;

    // Channel 0 = Left / Mid, Channel 1 = Right / Side
    std::array<std::array<DynamicBand, NUM_EQ_BANDS>, 2> bands_;
};

} // namespace strata::dsp
