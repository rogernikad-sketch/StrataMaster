#include "TransientProcessor.h"
#include <algorithm>
#include <cmath>

namespace strata::dsp {

void SingleBandTransient::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;

    // Fast envelope: 0.5 ms attack, 15 ms release
    fastAttCoeff_ = std::exp(-1.0f / (0.0005f * static_cast<float>(sampleRate)));
    fastRelCoeff_ = std::exp(-1.0f / (0.015f  * static_cast<float>(sampleRate)));

    // Slow envelope: 25 ms attack, 150 ms release
    slowAttCoeff_ = std::exp(-1.0f / (0.025f * static_cast<float>(sampleRate)));
    slowRelCoeff_ = std::exp(-1.0f / (0.150f * static_cast<float>(sampleRate)));

    reset();
}

void SingleBandTransient::reset() noexcept {
    fastEnv_ = 0.0f;
    slowEnv_ = 0.0f;
    gainSmoothed_ = 1.0f;
    activity_.store(0.0f, std::memory_order_relaxed);
}

void SingleBandTransient::processStereo(float* left, float* right, size_t numSamples) noexcept {
    if (!settings_.enabled || (std::abs(settings_.attackGainDb) < 0.01f && std::abs(settings_.sustainGainDb) < 0.01f)) {
        gainSmoothed_ = 1.0f;
        activity_.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const float kAtt = std::pow(10.0f, settings_.attackGainDb / 20.0f) - 1.0f;
    const float kSus = std::pow(10.0f, settings_.sustainGainDb / 20.0f) - 1.0f;

    float maxActivity = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        const float inL = left[i];
        const float inR = right[i];
        const float v = 0.5f * (std::abs(inL) + std::abs(inR));

        // Fast envelope update
        if (v > fastEnv_) {
            fastEnv_ = fastAttCoeff_ * fastEnv_ + (1.0f - fastAttCoeff_) * v;
        } else {
            fastEnv_ = fastRelCoeff_ * fastEnv_ + (1.0f - fastRelCoeff_) * v;
        }

        // Slow envelope update
        if (v > slowEnv_) {
            slowEnv_ = slowAttCoeff_ * slowEnv_ + (1.0f - slowAttCoeff_) * v;
        } else {
            slowEnv_ = slowRelCoeff_ * slowEnv_ + (1.0f - slowRelCoeff_) * v;
        }

        // Dynamic envelope differentiation with steady-state ripple squelch
        const float rippleThreshold = 0.15f * slowEnv_;
        const float deltaE = (fastEnv_ - slowEnv_) - rippleThreshold;
        const float transientEnergy = std::max(0.0f, deltaE);
        const float T = transientEnergy / (slowEnv_ + 1.0e-4f);
        const float S = slowEnv_ / (fastEnv_ + 1.0e-4f);

        const float tanh2T = std::tanh(2.5f * T);
        const float targetGain = 1.0f + kAtt * tanh2T + kSus * std::tanh(S) * (1.0f - tanh2T);
        const float clampedGain = std::clamp(targetGain, 0.1f, 4.0f);

        // Smooth gain modulation
        gainSmoothed_ = 0.92f * gainSmoothed_ + 0.08f * clampedGain;

        left[i]  *= gainSmoothed_;
        right[i] *= gainSmoothed_;

        const float act = gainSmoothed_ - 1.0f;
        if (std::abs(act) > std::abs(maxActivity)) {
            maxActivity = act;
        }
    }

    activity_.store(maxActivity, std::memory_order_relaxed);
}

// ==============================================================================
// 4-Band Transient Processor Implementation
// ==============================================================================

TransientProcessor::TransientProcessor() = default;

void TransientProcessor::prepare(double sampleRate, size_t maxBlockSize) {
    sampleRate_ = sampleRate;
    maxBlockSize_ = maxBlockSize;

    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        bands_[b].prepare(sampleRate);
    }
    widebandProcessor_.prepare(sampleRate);
    reset();
}

void TransientProcessor::reset() noexcept {
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        bands_[b].reset();
    }
    widebandProcessor_.reset();
}

void TransientProcessor::setBandSettings(size_t band, const BandTransientSettings& settings) noexcept {
    if (band < NUM_CROSSOVER_BANDS) {
        bands_[band].setSettings(settings);
    }
}

const BandTransientSettings& TransientProcessor::getBandSettings(size_t band) const noexcept {
    static const BandTransientSettings defaultSettings{};
    return (band < NUM_CROSSOVER_BANDS) ? bands_[band].getSettings() : defaultSettings;
}

float TransientProcessor::getBandActivity(size_t band) const noexcept {
    return (band < NUM_CROSSOVER_BANDS) ? bands_[band].getActivity() : 0.0f;
}

void TransientProcessor::processStereo(float* left, float* right, size_t numSamples,
                                      float attackGainDb, float sustainGainDb) noexcept {
    BandTransientSettings s;
    s.enabled = true;
    s.attackGainDb = attackGainDb;
    s.sustainGainDb = sustainGainDb;
    widebandProcessor_.setSettings(s);
    widebandProcessor_.processStereo(left, right, numSamples);
}

void TransientProcessor::processMultiband(float* bandBuffers[NUM_CROSSOVER_BANDS][2], size_t numSamples) noexcept {
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        bands_[b].processStereo(bandBuffers[b][0], bandBuffers[b][1], numSamples);
    }
}

} // namespace strata::dsp
