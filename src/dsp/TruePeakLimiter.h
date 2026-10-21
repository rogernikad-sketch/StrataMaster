#pragma once

#include "common/Constants.h"
#include "SoftClipper.h"
#include <vector>
#include <array>
#include <atomic>
#include <cstddef>
#include <cmath>

namespace strata::dsp {

/**
 * Professional Lookahead Brickwall True-Peak Limiter.
 * Compliant with ITU-R BS.1770-4 inter-sample peak detection.
 * Features:
 * - 3.0 ms Lookahead circular buffer (zero transient overshoots)
 * - 4x Polyphase inter-sample peak estimation
 * - Dual-stage adaptive release ballistics (transient transparency + pumping-free sustain)
 * - Integrated Pre-Limiter Soft Clipper
 * - Exact latency reporting and atomic gain reduction metering
 */
class TruePeakLimiter final {
public:
    TruePeakLimiter();
    ~TruePeakLimiter() = default;

    void prepare(double sampleRate, size_t maxBlockSize = 1024);
    void reset() noexcept;

    // Ceiling & Threshold controls
    void setCeilingDb(float ceilingDb) noexcept;
    [[nodiscard]] float getCeilingDb() const noexcept { return ceilingDb_; }

    void setThresholdDb(float thresholdDb) noexcept;
    [[nodiscard]] float getThresholdDb() const noexcept { return thresholdDb_; }

    void setReleaseMs(float releaseMs) noexcept;
    [[nodiscard]] float getReleaseMs() const noexcept { return releaseMs_; }

    void setLookaheadMs(float lookaheadMs) noexcept;
    [[nodiscard]] float getLookaheadMs() const noexcept { return lookaheadMs_; }

    [[nodiscard]] size_t getLatencyInSamples() const noexcept { return lookaheadSamples_; }
    [[nodiscard]] float getGainReductionDb() const noexcept {
        return gainReductionDb_.load(std::memory_order_relaxed);
    }

    // Pre-limiter Soft Clipper access
    [[nodiscard]] SoftClipper& getClipper() noexcept { return clipper_; }
    [[nodiscard]] const SoftClipper& getClipper() const noexcept { return clipper_; }

    void processStereo(float* left, float* right, size_t numSamples) noexcept;

private:
    void updateParameters() noexcept;
    [[nodiscard]] float estimateTruePeak(const float* history, size_t historyPos, size_t historyMask) const noexcept;

    double sampleRate_{ 44100.0 };
    float ceilingDb_{ -0.1f };    // dBTP
    float thresholdDb_{ 0.0f };   // Input drive/threshold in dB
    float releaseMs_{ 50.0f };    // Dual-stage release base time
    float lookaheadMs_{ 3.0f };   // Lookahead in ms

    float ceilingLinear_{ 0.98855f };
    float inputGain_{ 1.0f };

    size_t lookaheadSamples_{ 132 };
    size_t bufferSize_{ 2048 };
    size_t bufferMask_{ 2047 };

    // Circular delay buffers for Left and Right channels
    std::vector<float> delayBufferL_;
    std::vector<float> delayBufferR_;
    size_t writePos_{ 0 };

    // Gain envelope buffer for lookahead minimum tracking
    std::vector<float> gainBuffer_;
    float currentGain_{ 1.0f };
    float fastReleaseCoeff_{ 0.99f };
    float slowReleaseCoeff_{ 0.999f };

    SoftClipper clipper_;

    std::atomic<float> gainReductionDb_{ 0.0f };

    // 4-Phase Polyphase FIR Coefficients for 4x True-Peak Interpolation (16-tap Kaiser)
    static constexpr size_t INTERP_TAPS = 16;
    std::array<std::array<float, INTERP_TAPS>, 4> interpCoeffs_;
};

} // namespace strata::dsp
