#include "BiquadFilter.h"
#include <cmath>

namespace strata::dsp {

BiquadCoefficients BiquadFilter::calculateCoefficients(FilterType type, double sampleRate,
                                                      float frequencyHz, float gainDb, float q) noexcept {
    BiquadCoefficients coeffs;
    if (sampleRate <= 0.0) {
        return coeffs;
    }

    // Safety clamps
    const float maxFreq = static_cast<float>(sampleRate * 0.495);
    const float freq = std::clamp(frequencyHz, 10.0f, maxFreq);
    const float Q = std::clamp(q, 0.05f, 40.0f);
    const float A = std::pow(10.0f, gainDb * 0.025f); // 10^(gainDb / 40)

    const double omega = TWO_PI * freq / sampleRate;
    const double cosOmega = std::cos(omega);
    const double sinOmega = std::sin(omega);

    double alpha = 0.0;
    if (type == FilterType::LowShelf || type == FilterType::HighShelf) {
        // RBJ Cookbook shelving formula: Q=1.0 produces maximally flat Butterworth transition (S=1)
        const double term = (static_cast<double>(A) + 1.0 / static_cast<double>(A)) * (1.0 / static_cast<double>(Q) - 1.0) + 2.0;
        const double sqrtTerm = std::sqrt(std::max(0.0, term));
        alpha = (sinOmega * 0.5) * sqrtTerm;
    } else {
        alpha = sinOmega / (2.0 * static_cast<double>(Q));
    }

    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a0 = 1.0, a1 = 0.0, a2 = 0.0;

    switch (type) {
        case FilterType::Bell: {
            b0 = 1.0 + alpha * A;
            b1 = -2.0 * cosOmega;
            b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1 = -2.0 * cosOmega;
            a2 = 1.0 - alpha / A;
            break;
        }

        case FilterType::LowShelf: {
            const double sqrtA = std::sqrt(static_cast<double>(A));
            const double twoSqrtAAlpha = 2.0 * sqrtA * alpha;

            b0 = A * ((A + 1.0) - (A - 1.0) * cosOmega + twoSqrtAAlpha);
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosOmega);
            b2 = A * ((A + 1.0) - (A - 1.0) * cosOmega - twoSqrtAAlpha);
            a0 = (A + 1.0) + (A - 1.0) * cosOmega + twoSqrtAAlpha;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosOmega);
            a2 = (A + 1.0) + (A - 1.0) * cosOmega - twoSqrtAAlpha;
            break;
        }

        case FilterType::HighShelf: {
            const double sqrtA = std::sqrt(static_cast<double>(A));
            const double twoSqrtAAlpha = 2.0 * sqrtA * alpha;

            b0 = A * ((A + 1.0) + (A - 1.0) * cosOmega + twoSqrtAAlpha);
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosOmega);
            b2 = A * ((A + 1.0) + (A - 1.0) * cosOmega - twoSqrtAAlpha);
            a0 = (A + 1.0) - (A - 1.0) * cosOmega + twoSqrtAAlpha;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosOmega);
            a2 = (A + 1.0) - (A - 1.0) * cosOmega - twoSqrtAAlpha;
            break;
        }

        case FilterType::LowPass: {
            b0 = (1.0 - cosOmega) * 0.5;
            b1 = 1.0 - cosOmega;
            b2 = (1.0 - cosOmega) * 0.5;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosOmega;
            a2 = 1.0 - alpha;
            break;
        }

        case FilterType::HighPass: {
            b0 = (1.0 + cosOmega) * 0.5;
            b1 = -(1.0 + cosOmega);
            b2 = (1.0 + cosOmega) * 0.5;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosOmega;
            a2 = 1.0 - alpha;
            break;
        }

        case FilterType::Notch: {
            b0 = 1.0;
            b1 = -2.0 * cosOmega;
            b2 = 1.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosOmega;
            a2 = 1.0 - alpha;
            break;
        }

        case FilterType::BandPass: {
            b0 = alpha;
            b1 = 0.0;
            b2 = -alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosOmega;
            a2 = 1.0 - alpha;
            break;
        }

        case FilterType::AllPass: {
            b0 = 1.0 - alpha;
            b1 = -2.0 * cosOmega;
            b2 = 1.0 + alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cosOmega;
            a2 = 1.0 - alpha;
            break;
        }
    }

    if (std::abs(a0) > 1.0e-9) {
        const double invA0 = 1.0 / a0;
        coeffs.b0 = static_cast<float>(b0 * invA0);
        coeffs.b1 = static_cast<float>(b1 * invA0);
        coeffs.b2 = static_cast<float>(b2 * invA0);
        coeffs.a1 = static_cast<float>(a1 * invA0);
        coeffs.a2 = static_cast<float>(a2 * invA0);
    }

    return coeffs;
}

void BiquadFilter::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
}

void BiquadFilter::reset() noexcept {
    s1_ = 0.0f;
    s2_ = 0.0f;
    snapToTarget();
}

void BiquadFilter::snapToTarget() noexcept {
    currentCoeffs_ = targetCoeffs_;
    isInterpolating_ = false;
    interpolationSamplesLeft_ = 0;
}

void BiquadFilter::setParameters(FilterType type, float frequencyHz, float gainDb, float q, bool snapImmediately) noexcept {
    setCoefficients(calculateCoefficients(type, sampleRate_, frequencyHz, gainDb, q), snapImmediately);
}

void BiquadFilter::setCoefficients(const BiquadCoefficients& newCoeffs, bool snapImmediately) noexcept {
    targetCoeffs_ = newCoeffs;

    if (snapImmediately) {
        snapToTarget();
        return;
    }

    // Smooth transition over 64 samples (~1.4 ms at 44.1kHz)
    constexpr size_t rampLength = 64;
    interpolationSamplesLeft_ = rampLength;
    const float invRamp = 1.0f / static_cast<float>(rampLength);

    coeffStep_.b0 = (targetCoeffs_.b0 - currentCoeffs_.b0) * invRamp;
    coeffStep_.b1 = (targetCoeffs_.b1 - currentCoeffs_.b1) * invRamp;
    coeffStep_.b2 = (targetCoeffs_.b2 - currentCoeffs_.b2) * invRamp;
    coeffStep_.a1 = (targetCoeffs_.a1 - currentCoeffs_.a1) * invRamp;
    coeffStep_.a2 = (targetCoeffs_.a2 - currentCoeffs_.a2) * invRamp;

    isInterpolating_ = true;
}

} // namespace strata::dsp
