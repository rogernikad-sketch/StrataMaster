#pragma once

#include <algorithm>
#include <cmath>

namespace strata::dsp {

enum class SmoothingType {
    Linear,
    Exponential
};

/**
 * Click-free parameter smoother supporting linear and exponential ballistics.
 */
template <typename FloatType = float>
class SmoothedValue final {
public:
    SmoothedValue() noexcept = default;

    explicit SmoothedValue(FloatType initialValue) noexcept
        : currentValue_(initialValue), targetValue_(initialValue) {}

    void reset(double sampleRate, double rampTimeSeconds) noexcept {
        sampleRate_ = sampleRate;
        rampTimeSeconds_ = rampTimeSeconds;
        updateStep();
        currentValue_ = targetValue_;
    }

    void setTargetValue(FloatType newTarget) noexcept {
        if (targetValue_ == newTarget) {
            return;
        }
        targetValue_ = newTarget;
        updateStep();
    }

    void setCurrentAndTargetValue(FloatType value) noexcept {
        currentValue_ = value;
        targetValue_ = value;
    }

    [[nodiscard]] FloatType getTargetValue() const noexcept {
        return targetValue_;
    }

    [[nodiscard]] FloatType getCurrentValue() const noexcept {
        return currentValue_;
    }

    [[nodiscard]] bool isSmoothing() const noexcept {
        return std::abs(targetValue_ - currentValue_) > FloatType(1e-6);
    }

    void setSmoothingType(SmoothingType type) noexcept {
        type_ = type;
        updateStep();
    }

    FloatType getNextValue() noexcept {
        if (!isSmoothing()) {
            currentValue_ = targetValue_;
            return currentValue_;
        }

        if (type_ == SmoothingType::Linear) {
            if (currentValue_ < targetValue_) {
                currentValue_ = std::min(targetValue_, currentValue_ + step_);
            } else {
                currentValue_ = std::max(targetValue_, currentValue_ - step_);
            }
        } else {
            currentValue_ += step_ * (targetValue_ - currentValue_);
        }

        return currentValue_;
    }

    void skip(int numSamples) noexcept {
        for (int i = 0; i < numSamples && isSmoothing(); ++i) {
            getNextValue();
        }
    }

private:
    void updateStep() noexcept {
        if (sampleRate_ <= 0.0 || rampTimeSeconds_ <= 0.0) {
            step_ = FloatType(1.0);
            return;
        }

        const auto numSamples = static_cast<FloatType>(sampleRate_ * rampTimeSeconds_);
        if (type_ == SmoothingType::Linear) {
            step_ = std::abs(targetValue_ - currentValue_) / std::max(FloatType(1.0), numSamples);
        } else {
            // Exponential smoothing coefficient for 99.9% settling time
            step_ = FloatType(1.0) - std::exp(FloatType(-6.907755) / std::max(FloatType(1.0), numSamples));
        }
    }

    FloatType currentValue_{FloatType(0)};
    FloatType targetValue_{FloatType(0)};
    FloatType step_{FloatType(1.0)};
    double sampleRate_{44100.0};
    double rampTimeSeconds_{0.02}; // 20ms default
    SmoothingType type_{SmoothingType::Exponential};
};

} // namespace strata::dsp
