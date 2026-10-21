#pragma once

#include "Oversampler.h"
#include "CrossoverFilterbank.h"
#include <array>
#include <cstddef>

namespace strata::dsp {

enum class SaturationMode {
    Tape = 0,
    Tube,
    Warm,
    Retro
};

struct BandExciterSettings {
    bool enabled{ true };
    SaturationMode mode{ SaturationMode::Tape };
    float drive{ 0.0f }; // 0.0 (clean) to 1.0 (+24 dB boost)
    float mix{ 1.0f };   // 0.0 (dry) to 1.0 (wet)
};

/**
 * 1st-order DC-blocking high-pass filter for asymmetric saturation modes.
 * Cutoff ~ 5 Hz.
 */
class DCBlocker {
public:
    void prepare(double sampleRate) noexcept {
        // R = 1.0 - (2 * pi * fc / fs)
        const double fc = 5.0;
        const double twoPi = 6.283185307179586;
        R_ = static_cast<float>(1.0 - (twoPi * fc / sampleRate));
        reset();
    }

    void reset() noexcept {
        xPrev_ = 0.0f;
        yPrev_ = 0.0f;
    }

    [[nodiscard]] float processSample(float x) noexcept {
        const float y = x - xPrev_ + R_ * yPrev_;
        xPrev_ = x;
        yPrev_ = y;
        return y;
    }

private:
    float R_{ 0.9993f };
    float xPrev_{ 0.0f };
    float yPrev_{ 0.0f };
};

/**
 * Professional Harmonic Exciter & Multiband Saturator.
 * Features 4 saturation algorithms (Tape, Tube, Warm, Retro),
 * 4 frequency bands integrated with the crossover filterbank,
 * and polyphase oversampling up to 8x to eliminate foldover aliasing.
 */
class Saturator {
public:
    Saturator();
    ~Saturator() = default;

    void prepare(double sampleRate, size_t maxBlockSize, OversamplingFactor factor = OversamplingFactor::X4);
    void reset() noexcept;

    void setOversamplingFactor(OversamplingFactor factor);
    [[nodiscard]] OversamplingFactor getOversamplingFactor() const noexcept { return oversampler_.getFactor(); }
    [[nodiscard]] size_t getLatencyInSamples() const noexcept { return oversampler_.getLatencyInSamples(); }

    void setBandSettings(size_t band, const BandExciterSettings& settings) noexcept;
    [[nodiscard]] const BandExciterSettings& getBandSettings(size_t band) const noexcept;

    /**
     * Processes wideband stereo audio through a single saturation algorithm.
     */
    void processStereo(float* left, float* right, size_t numSamples,
                       SaturationMode mode, float drive, float mix) noexcept;

    /**
     * Processes 4-band split audio (from CrossoverFilterbank).
     * bandBuffers[b][ch]: pointer to audio buffer for band b (0..3) and channel ch (0..1).
     */
    void processMultiband(float* bandBuffers[NUM_CROSSOVER_BANDS][2], size_t numSamples) noexcept;

    /**
     * Static single-sample waveshaping functions.
     */
    [[nodiscard]] static float shapeTape(float x, float drive) noexcept;
    [[nodiscard]] static float shapeTube(float x, float drive, DCBlocker& dc) noexcept;
    [[nodiscard]] static float shapeWarm(float x, float drive, DCBlocker& dc) noexcept;
    [[nodiscard]] static float shapeRetro(float x, float drive) noexcept;

private:
    void processOversampledBand(float* left, float* right, size_t numSamples,
                                SaturationMode mode, float drive, float mix,
                                size_t bandIdx) noexcept;

    double sampleRate_{ 44100.0 };
    size_t maxBlockSize_{ 1024 };

    Oversampler oversampler_;

    std::array<BandExciterSettings, NUM_CROSSOVER_BANDS> bandSettings_;

    // DC blockers for asymmetric modes (Tube, Warm): 4 bands x 2 channels
    std::array<std::array<DCBlocker, 2>, NUM_CROSSOVER_BANDS> dcBlockers_;
    // DC blockers for wideband processing
    std::array<DCBlocker, 2> widebandDCBlockers_;

    // Temporary working buffers for wideband oversampling
    std::vector<float> workBufferL_;
    std::vector<float> workBufferR_;
};

} // namespace strata::dsp
