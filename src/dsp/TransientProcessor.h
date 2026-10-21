#pragma once

#include "CrossoverFilterbank.h"
#include <array>
#include <atomic>
#include <cstddef>

namespace strata::dsp {

struct BandTransientSettings {
    bool enabled{ true };
    float attackGainDb{ 0.0f };  // -12.0 dB to +12.0 dB
    float sustainGainDb{ 0.0f }; // -12.0 dB to +12.0 dB
};

/**
 * Single-band dynamic envelope differentiation transient & sustain shaper.
 */
class SingleBandTransient {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setSettings(const BandTransientSettings& settings) noexcept { settings_ = settings; }
    [[nodiscard]] const BandTransientSettings& getSettings() const noexcept { return settings_; }
    [[nodiscard]] float getActivity() const noexcept { return activity_.load(std::memory_order_relaxed); }

    void processStereo(float* left, float* right, size_t numSamples) noexcept;

private:
    BandTransientSettings settings_{};

    double sampleRate_{ 44100.0 };
    float fastEnv_{ 0.0f };
    float slowEnv_{ 0.0f };
    float gainSmoothed_{ 1.0f };

    float fastAttCoeff_{ 0.0f };
    float fastRelCoeff_{ 0.0f };
    float slowAttCoeff_{ 0.0f };
    float slowRelCoeff_{ 0.0f };

    std::atomic<float> activity_{ 0.0f };
};

/**
 * 4-Band Microdynamics Transient & Sustain Processor.
 * Provides independent microdynamic punch and sustain shaping across the 4 crossover bands.
 */
class TransientProcessor {
public:
    TransientProcessor();
    ~TransientProcessor() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset() noexcept;

    void setBandSettings(size_t band, const BandTransientSettings& settings) noexcept;
    [[nodiscard]] const BandTransientSettings& getBandSettings(size_t band) const noexcept;
    [[nodiscard]] float getBandActivity(size_t band) const noexcept;

    /**
     * Processes wideband stereo audio.
     */
    void processStereo(float* left, float* right, size_t numSamples,
                       float attackGainDb, float sustainGainDb) noexcept;

    /**
     * Processes 4-band split audio (from CrossoverFilterbank).
     * bandBuffers[b][ch]: pointer to audio buffer for band b (0..3) and channel ch (0..1).
     */
    void processMultiband(float* bandBuffers[NUM_CROSSOVER_BANDS][2], size_t numSamples) noexcept;

private:
    double sampleRate_{ 44100.0 };
    size_t maxBlockSize_{ 1024 };

    std::array<SingleBandTransient, NUM_CROSSOVER_BANDS> bands_;
    SingleBandTransient widebandProcessor_;
};

} // namespace strata::dsp
