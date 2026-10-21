#pragma once

#include "CrossoverFilterbank.h"
#include "common/Constants.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <vector>

namespace strata::dsp {

struct BandDynamicsSettings {
    bool enabled{true};
    bool bypass{false};
    bool solo{false};
    bool mute{false};

    float thresholdDb{-18.0f}; // dBFS [-60, 0]
    float ratio{2.0f};         // [1.0, 30.0]
    float attackMs{15.0f};      // [0.1, 250.0]
    float releaseMs{100.0f};   // [10.0, 1000.0]
    float kneeWidthDb{3.0f};   // [0.0, 12.0]
    float makeupGainDb{0.0f};  // [-18.0, 18.0]
    bool autoMakeup{false};
};

/**
 * Single-channel dynamic compressor/expander with soft knee.
 */
class SingleBandCompressor final {
public:
    SingleBandCompressor() noexcept = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void updateSettings(const BandDynamicsSettings& settings) noexcept;

    /** Processes audio buffer in-place */
    void process(float* buffer, size_t numSamples) noexcept;

    [[nodiscard]] float getGainReductionDb() const noexcept {
        return gainReductionDb_.load(std::memory_order_relaxed);
    }

private:
    double sampleRate_{44100.0};
    BandDynamicsSettings settings_;

    float envelopeLevel_{0.0f};
    float smoothedGainReductionDb_{0.0f};
    float attackCoeff_{0.0f};
    float releaseCoeff_{0.0f};

    std::atomic<float> gainReductionDb_{0.0f};
};

/**
 * 4-Band Multiband Dynamics Processor.
 * Integrates 4-band phase-compensated crossover filterbank with independent dynamics.
 */
class MultibandDynamics final {
public:
    MultibandDynamics();

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setCrossoverFrequencies(float f1, float f2, float f3, bool snapImmediately = false) noexcept {
        crossover_.setCrossoverFrequencies(f1, f2, f3, snapImmediately);
    }

    [[nodiscard]] std::array<float, NUM_CROSSOVER_SPLITS> getCrossoverFrequencies() const noexcept {
        return crossover_.getCrossoverFrequencies();
    }

    void setBandSettings(size_t bandIndex, const BandDynamicsSettings& settings) noexcept;
    [[nodiscard]] const BandDynamicsSettings& getBandSettings(size_t bandIndex) const noexcept;

    /** Processes stereo audio block in-place */
    void processStereo(float* left, float* right, size_t numSamples) noexcept;

    /** Gets current gain reduction in dB for a band (channel 0 = L, 1 = R) */
    [[nodiscard]] float getGainReductionDb(size_t bandIndex, size_t channel = 0) const noexcept;

private:
    double sampleRate_{44100.0};
    CrossoverFilterbank crossover_;

    std::array<BandDynamicsSettings, NUM_CROSSOVER_BANDS> bandSettings_;
    // Channel 0 = Left, Channel 1 = Right
    std::array<std::array<SingleBandCompressor, NUM_CROSSOVER_BANDS>, 2> compressors_;

    // Internal scratch buffers for 4 bands per channel
    std::array<std::vector<float>, NUM_CROSSOVER_BANDS> leftBandBuffers_;
    std::array<std::vector<float>, NUM_CROSSOVER_BANDS> rightBandBuffers_;
};

} // namespace strata::dsp
