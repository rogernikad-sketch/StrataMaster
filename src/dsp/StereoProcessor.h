#pragma once

#include "common/Constants.h"
#include "BiquadFilter.h"
#include "CrossoverFilterbank.h"
#include <array>
#include <vector>
#include <cstddef>

namespace strata::dsp {

enum class StereoMode {
    Stereo,
    MidSide,
    MidSolo,
    SideSolo
};

/**
 * Advanced Stereo & Mid/Side Imager.
 * Features:
 * - 0 dB power-conserving Mid/Side matrix
 * - 4-Band stereo width scaling
 * - Mono-Maker: High-pass filtered Side channel for vinyl/club bass mono-compatibility
 * - Stereoizer: Psychoacoustic Haas decorrelator with 100% mono cancellation guarantee
 */
class StereoProcessor final {
public:
    StereoProcessor();
    ~StereoProcessor() = default;

    void prepare(double sampleRate, size_t maxBlockSize = 1024) noexcept;
    void reset() noexcept;

    /** Encodes L/R stereo into M/S with 0 dB power conservation. */
    static void encodeMidSide(float* left, float* right, size_t numSamples) noexcept;

    /** Decodes M/S back into L/R stereo with bit-exact reconstruction. */
    static void decodeMidSide(float* mid, float* side, size_t numSamples) noexcept;

    /** Applies stereo width scaling [0.0 = mono, 1.0 = unity, 2.0 = extra wide] */
    static void applyWidth(float* left, float* right, size_t numSamples, float width) noexcept;

    // Mono-maker controls
    void setMonoMakerEnabled(bool enabled) noexcept { monoMakerEnabled_ = enabled; }
    [[nodiscard]] bool isMonoMakerEnabled() const noexcept { return monoMakerEnabled_; }
    void setMonoMakerFrequency(float freqHz) noexcept;
    [[nodiscard]] float getMonoMakerFrequency() const noexcept { return monoMakerFreq_; }

    // Stereoizer controls
    void setStereoizerEnabled(bool enabled) noexcept { stereoizerEnabled_ = enabled; }
    [[nodiscard]] bool isStereoizerEnabled() const noexcept { return stereoizerEnabled_; }
    void setStereoizerAmount(float amount) noexcept { stereoizerAmount_ = amount; } // 0.0 to 1.0
    [[nodiscard]] float getStereoizerAmount() const noexcept { return stereoizerAmount_; }

    /** Full stereo processing pipeline: Mono-maker -> Width -> Stereoizer -> Mode output */
    void process(float* left, float* right, size_t numSamples, StereoMode mode, float width) noexcept;

    /** 4-Band Width processing across crossover bands */
    static void applyMultibandWidth(float* bandBuffers[NUM_CROSSOVER_BANDS][2],
                                   size_t numSamples,
                                   const std::array<float, NUM_CROSSOVER_BANDS>& widths) noexcept;

private:
    double sampleRate_{ 44100.0 };
    bool monoMakerEnabled_{ false };
    float monoMakerFreq_{ 120.0f };

    bool stereoizerEnabled_{ false };
    float stereoizerAmount_{ 0.0f };

    // 2nd-order Linkwitz-Riley / Butterworth high-pass filter for Side channel
    BiquadFilter sideHPF_;

    // Delay buffer for Haas / Stereoizer decorrelation on Side channel
    std::vector<float> haasBuffer_;
    size_t haasWritePos_{ 0 };
    size_t haasDelaySamples_{ 88 }; // ~2.0 ms delay at 44.1 kHz
};

} // namespace strata::dsp
