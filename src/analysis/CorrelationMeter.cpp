#include "CorrelationMeter.h"
#include <algorithm>
#include <cmath>

namespace strata::analysis {

void CorrelationMeter::prepare(double sampleRate) noexcept {
    // 150 ms smoothing time constant
    if (sampleRate > 0.0) {
        filterCoeff_ = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate * 0.15)));
    }
    reset();
}

void CorrelationMeter::reset() noexcept {
    sumLR_ = 0.0f;
    sumLL_ = 0.0f;
    sumRR_ = 0.0f;
    correlationValue_.store(1.0f, std::memory_order_relaxed);
}

void CorrelationMeter::process(const float* left, const float* right, size_t numSamples) noexcept {
    float blockLR = 0.0f;
    float blockLL = 0.0f;
    float blockRR = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        const float l = left[i];
        const float r = right[i];
        blockLR += l * r;
        blockLL += l * l;
        blockRR += r * r;
    }

    sumLR_ += filterCoeff_ * (blockLR - sumLR_);
    sumLL_ += filterCoeff_ * (blockLL - sumLL_);
    sumRR_ += filterCoeff_ * (blockRR - sumRR_);

    const float denom = std::sqrt(sumLL_ * sumRR_);
    float corr = 1.0f;
    if (denom > 1.0e-7f) {
        corr = sumLR_ / denom;
        corr = std::clamp(corr, -1.0f, 1.0f);
    }

    correlationValue_.store(corr, std::memory_order_relaxed);
}

} // namespace strata::analysis
