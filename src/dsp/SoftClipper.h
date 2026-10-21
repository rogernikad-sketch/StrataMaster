#pragma once

#include "common/Constants.h"
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace strata::dsp {

/**
 * Mastering Pre-Limiter Soft Clipper.
 * Features quadratic soft-knee saturation that shaves aggressive transient peaks
 * with C^1 continuous derivative and absolute ceiling constraint.
 */
class SoftClipper final {
public:
    SoftClipper() noexcept {
        updateCoefficients();
    }

    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }

    void setCeilingDb(float ceilingDb) noexcept {
        ceilingDb_ = std::clamp(ceilingDb, -12.0f, 0.0f);
        updateCoefficients();
    }
    [[nodiscard]] float getCeilingDb() const noexcept { return ceilingDb_; }

    void setKneeDb(float kneeDb) noexcept {
        kneeDb_ = std::clamp(kneeDb, 0.0f, 6.0f);
        updateCoefficients();
    }
    [[nodiscard]] float getKneeDb() const noexcept { return kneeDb_; }

    [[nodiscard]] float processSample(float x) const noexcept {
        if (!enabled_) {
            return x;
        }

        const float absX = std::abs(x);
        if (absX <= kneeLinear_) {
            return x;
        }

        const float sign = (x >= 0.0f) ? 1.0f : -1.0f;
        const float d = absX - kneeLinear_;

        if (d >= doubleSpan_) {
            return sign * ceilingLinear_;
        }

        const float shaped = kneeLinear_ + d - (d * d) * invFourSpan_;
        return sign * shaped;
    }

    void processStereo(float* left, float* right, size_t numSamples) const noexcept {
        if (!enabled_) {
            return;
        }

        for (size_t i = 0; i < numSamples; ++i) {
            left[i]  = processSample(left[i]);
            right[i] = processSample(right[i]);
        }
    }

private:
    void updateCoefficients() noexcept {
        ceilingLinear_ = std::pow(10.0f, ceilingDb_ / 20.0f);
        const float kneeStartDb = ceilingDb_ - kneeDb_;
        kneeLinear_ = std::pow(10.0f, kneeStartDb / 20.0f);

        const float span = std::max(1.0e-5f, ceilingLinear_ - kneeLinear_);
        doubleSpan_ = 2.0f * span;
        invFourSpan_ = 1.0f / (4.0f * span);
    }

    bool enabled_{ false };
    float ceilingDb_{ -0.5f };
    float kneeDb_{ 2.0f };

    float ceilingLinear_{ 0.944f };
    float kneeLinear_{ 0.750f };
    float doubleSpan_{ 0.388f };
    float invFourSpan_{ 1.288f };
};

} // namespace strata::dsp
