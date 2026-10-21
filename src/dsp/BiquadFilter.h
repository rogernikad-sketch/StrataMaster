#pragma once

#include "common/Constants.h"
#include <algorithm>
#include <cmath>
#include <complex>

namespace strata::dsp {

enum class FilterType {
    Bell,
    LowShelf,
    HighShelf,
    LowPass,
    HighPass,
    Notch,
    BandPass,
    AllPass
};

/**
 * Second-order Biquad Filter Coefficients.
 * Normalized such that a0 = 1.0.
 */
struct BiquadCoefficients {
    float b0{1.0f};
    float b1{0.0f};
    float b2{0.0f};
    float a1{0.0f};
    float a2{0.0f};

    /** Computes complex frequency response H(e^{j omega}) at frequency f for sampleRate */
    [[nodiscard]] std::complex<double> getComplexResponse(double frequencyHz, double sampleRate) const noexcept {
        const double omega = TWO_PI * frequencyHz / sampleRate;
        const double cos1 = std::cos(omega);
        const double sin1 = std::sin(omega);
        const double cos2 = std::cos(2.0 * omega);
        const double sin2 = std::sin(2.0 * omega);

        const std::complex<double> num(b0 + b1 * cos1 + b2 * cos2, -b1 * sin1 - b2 * sin2);
        const std::complex<double> den(1.0 + a1 * cos1 + a2 * cos2, -a1 * sin1 - a2 * sin2);

        return num / den;
    }

    /** Computes magnitude gain response at frequencyHz */
    [[nodiscard]] double getMagnitudeResponse(double frequencyHz, double sampleRate) const noexcept {
        return std::abs(getComplexResponse(frequencyHz, sampleRate));
    }

    /** Computes magnitude gain response in decibels */
    [[nodiscard]] double getMagnitudeResponseDb(double frequencyHz, double sampleRate) const noexcept {
        const double mag = getMagnitudeResponse(frequencyHz, sampleRate);
        return (mag > 1.0e-5) ? (20.0 * std::log10(mag)) : -100.0;
    }
};

/**
 * High-performance Direct Form II Transposed Biquad Filter.
 * Numerical stability, zero-latency, minimum quantization noise.
 */
class BiquadFilter final {
public:
    BiquadFilter() noexcept = default;

    /** Calculates coefficients for given filter parameters according to RBJ Audio EQ Cookbook */
    static BiquadCoefficients calculateCoefficients(FilterType type, double sampleRate,
                                                    float frequencyHz, float gainDb, float q) noexcept;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setParameters(FilterType type, float frequencyHz, float gainDb, float q, bool snapImmediately = false) noexcept;
    void setCoefficients(const BiquadCoefficients& newCoeffs, bool snapImmediately = false) noexcept;
    void snapToTarget() noexcept;

    [[nodiscard]] const BiquadCoefficients& getCoefficients() const noexcept { return targetCoeffs_; }

    /** Processes a single sample in-place */
    [[nodiscard]] inline float processSample(float input) noexcept {
        if (isInterpolating_) {
            advanceInterpolation();
        }
        // Direct Form II Transposed difference equation
        const float output = currentCoeffs_.b0 * input + s1_;
        s1_ = currentCoeffs_.b1 * input - currentCoeffs_.a1 * output + s2_;
        s2_ = currentCoeffs_.b2 * input - currentCoeffs_.a2 * output;
        return output;
    }

    /** Processes an audio block in-place */
    void processBlock(float* buffer, size_t numSamples) noexcept {
        for (size_t i = 0; i < numSamples; ++i) {
            buffer[i] = processSample(buffer[i]);
        }
    }

    /** Computes magnitude response at frequencyHz for UI spectrum overlay */
    [[nodiscard]] double getMagnitudeResponseDb(double frequencyHz) const noexcept {
        return targetCoeffs_.getMagnitudeResponseDb(frequencyHz, sampleRate_);
    }

private:
    inline void advanceInterpolation() noexcept {
        if (interpolationSamplesLeft_ > 0) {
            currentCoeffs_.b0 += coeffStep_.b0;
            currentCoeffs_.b1 += coeffStep_.b1;
            currentCoeffs_.b2 += coeffStep_.b2;
            currentCoeffs_.a1 += coeffStep_.a1;
            currentCoeffs_.a2 += coeffStep_.a2;
            --interpolationSamplesLeft_;
        } else {
            currentCoeffs_ = targetCoeffs_;
            isInterpolating_ = false;
        }
    }

    double sampleRate_{44100.0};
    BiquadCoefficients currentCoeffs_;
    BiquadCoefficients targetCoeffs_;
    BiquadCoefficients coeffStep_;

    // Transposed Direct Form II state variables
    float s1_{0.0f};
    float s2_{0.0f};

    bool isInterpolating_{false};
    size_t interpolationSamplesLeft_{0};
};

} // namespace strata::dsp
