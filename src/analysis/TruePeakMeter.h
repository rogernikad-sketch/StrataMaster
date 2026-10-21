#pragma once

#include <vector>
#include <array>
#include <atomic>
#include <cstddef>
#include <cmath>

namespace strata::analysis {

/**
 * ITU-R BS.1770-4 Compliant True-Peak Meter.
 * Provides:
 * - 4x Polyphase FIR inter-sample peak detection
 * - Left and Right real-time dBTP values
 * - Max True-Peak Hold with decay ballistics
 */
class TruePeakMeter final {
public:
    TruePeakMeter();
    ~TruePeakMeter() = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void processStereo(const float* left, const float* right, size_t numSamples) noexcept;

    [[nodiscard]] float getTruePeakDbL() const noexcept {
        return truePeakDbL_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] float getTruePeakDbR() const noexcept {
        return truePeakDbR_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] float getMaxTruePeakDb() const noexcept {
        return maxTruePeakDb_.load(std::memory_order_relaxed);
    }

    void resetMaxHold() noexcept {
        maxTruePeakDb_.store(-100.0f, std::memory_order_relaxed);
    }

private:
    [[nodiscard]] float estimateTruePeak(const float* buffer, size_t pos, size_t mask) const noexcept;

    double sampleRate_{ 44100.0 };

    static constexpr size_t INTERP_TAPS = 16;
    std::array<std::array<float, INTERP_TAPS>, 4> interpCoeffs_;

    size_t bufferSize_{ 64 };
    size_t bufferMask_{ 63 };
    std::vector<float> historyL_;
    std::vector<float> historyR_;
    size_t writePos_{ 0 };

    std::atomic<float> truePeakDbL_{ -100.0f };
    std::atomic<float> truePeakDbR_{ -100.0f };
    std::atomic<float> maxTruePeakDb_{ -100.0f };

    float currentPeakL_{ 0.0f };
    float currentPeakR_{ 0.0f };
    float releaseCoeff_{ 0.999f };
};

} // namespace strata::analysis
