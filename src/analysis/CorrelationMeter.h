#pragma once

#include <atomic>
#include <cstddef>

namespace strata::analysis {

/**
 * Computes stereo phase correlation coefficient:
 *   +1.0 = Pure mono / in-phase
 *    0.0 = Uncorrelated stereo
 *   -1.0 = Pure out-of-phase / antiphase
 */
class CorrelationMeter final {
public:
    CorrelationMeter() noexcept = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void process(const float* left, const float* right, size_t numSamples) noexcept;

    [[nodiscard]] float getCorrelation() const noexcept {
        return correlationValue_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<float> correlationValue_{1.0f};
    float sumLR_{0.0f};
    float sumLL_{0.0f};
    float sumRR_{0.0f};
    float filterCoeff_{0.05f};
};

} // namespace strata::analysis
