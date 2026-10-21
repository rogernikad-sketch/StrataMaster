#include "MultibandDynamics.h"
#include "common/DecibelUtils.h"
#include <algorithm>
#include <cmath>

namespace strata::dsp {

// ==============================================================================
// SingleBandCompressor Implementation
// ==============================================================================

void SingleBandCompressor::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
    updateSettings(settings_);
}

void SingleBandCompressor::reset() noexcept {
    envelopeLevel_ = 0.0f;
    smoothedGainReductionDb_ = 0.0f;
    gainReductionDb_.store(0.0f, std::memory_order_relaxed);
}

void SingleBandCompressor::updateSettings(const BandDynamicsSettings& settings) noexcept {
    settings_ = settings;
    if (sampleRate_ <= 0.0) {
        return;
    }

    const float attSec = std::max(0.0001f, settings_.attackMs * 0.001f);
    const float relSec = std::max(0.001f, settings_.releaseMs * 0.001f);

    attackCoeff_  = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate_ * attSec)));
    releaseCoeff_ = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate_ * relSec)));
}

void SingleBandCompressor::process(float* buffer, size_t numSamples) noexcept {
    if (!settings_.enabled || settings_.bypass || numSamples == 0) {
        gainReductionDb_.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const float T = settings_.thresholdDb;
    const float W = std::max(0.01f, settings_.kneeWidthDb);
    const float invRatio = 1.0f / std::max(0.1f, settings_.ratio);
    const float slope = invRatio - 1.0f;

    float autoMakeupOffsetDb = 0.0f;
    if (settings_.autoMakeup && settings_.ratio > 1.0f) {
        autoMakeupOffsetDb = -0.5f * T * (1.0f - invRatio);
    }
    const float staticGainDb = settings_.makeupGainDb + autoMakeupOffsetDb;

    for (size_t i = 0; i < numSamples; ++i) {
        const float inSample = buffer[i];
        const float absIn = std::abs(inSample);

        // Peak envelope follower with ballistics
        if (absIn > envelopeLevel_) {
            envelopeLevel_ += attackCoeff_ * (absIn - envelopeLevel_);
        } else {
            envelopeLevel_ += releaseCoeff_ * (absIn - envelopeLevel_);
        }

        const float envDb = gainToDecibels(envelopeLevel_, -96.0f);
        const float delta = envDb - T;

        float targetGrDb = 0.0f;
        if (2.0f * delta < -W) {
            targetGrDb = 0.0f; // Below knee
        } else if (2.0f * std::abs(delta) <= W) {
            // Inside soft knee
            const float kneeFactor = delta + W * 0.5f;
            targetGrDb = slope * (kneeFactor * kneeFactor) / (2.0f * W);
        } else {
            // Above knee
            targetGrDb = slope * delta;
        }

        // Clamp to negative for downward compression
        if (settings_.ratio >= 1.0f) {
            targetGrDb = std::min(0.0f, targetGrDb);
        }

        // Smooth gain reduction transitions
        if (targetGrDb < smoothedGainReductionDb_) {
            smoothedGainReductionDb_ += attackCoeff_ * (targetGrDb - smoothedGainReductionDb_);
        } else {
            smoothedGainReductionDb_ += releaseCoeff_ * (targetGrDb - smoothedGainReductionDb_);
        }

        const float totalGainLinear = decibelsToGain(smoothedGainReductionDb_ + staticGainDb);
        buffer[i] = inSample * totalGainLinear;
    }

    gainReductionDb_.store(smoothedGainReductionDb_, std::memory_order_relaxed);
}

// ==============================================================================
// MultibandDynamics Implementation
// ==============================================================================

MultibandDynamics::MultibandDynamics() {
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        bandSettings_[b].enabled = true;
        bandSettings_[b].bypass = false;
        bandSettings_[b].solo = false;
        bandSettings_[b].mute = false;
        bandSettings_[b].thresholdDb = -18.0f;
        bandSettings_[b].ratio = 2.0f;
        bandSettings_[b].attackMs = 15.0f;
        bandSettings_[b].releaseMs = 100.0f;
        bandSettings_[b].kneeWidthDb = 3.0f;
        bandSettings_[b].makeupGainDb = 0.0f;
        bandSettings_[b].autoMakeup = false;
    }
}

void MultibandDynamics::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
    crossover_.prepare(sampleRate);

    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        leftBandBuffers_[b].resize(2048, 0.0f);
        rightBandBuffers_[b].resize(2048, 0.0f);
        compressors_[0][b].prepare(sampleRate);
        compressors_[1][b].prepare(sampleRate);
        compressors_[0][b].updateSettings(bandSettings_[b]);
        compressors_[1][b].updateSettings(bandSettings_[b]);
    }
}

void MultibandDynamics::reset() noexcept {
    crossover_.reset();
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        compressors_[0][b].reset();
        compressors_[1][b].reset();
    }
}

void MultibandDynamics::setBandSettings(size_t bandIndex, const BandDynamicsSettings& settings) noexcept {
    if (bandIndex >= NUM_CROSSOVER_BANDS) {
        return;
    }
    bandSettings_[bandIndex] = settings;
    compressors_[0][bandIndex].updateSettings(settings);
    compressors_[1][bandIndex].updateSettings(settings);
}

const BandDynamicsSettings& MultibandDynamics::getBandSettings(size_t bandIndex) const noexcept {
    static const BandDynamicsSettings fallback;
    if (bandIndex >= NUM_CROSSOVER_BANDS) {
        return fallback;
    }
    return bandSettings_[bandIndex];
}

void MultibandDynamics::processStereo(float* left, float* right, size_t numSamples) noexcept {
    if (numSamples == 0) {
        return;
    }

    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        if (leftBandBuffers_[b].size() < numSamples) {
            leftBandBuffers_[b].resize(numSamples);
            rightBandBuffers_[b].resize(numSamples);
        }
    }

    std::array<float*, NUM_CROSSOVER_BANDS> bandPtrsL = {
        leftBandBuffers_[0].data(), leftBandBuffers_[1].data(),
        leftBandBuffers_[2].data(), leftBandBuffers_[3].data()
    };
    std::array<float*, NUM_CROSSOVER_BANDS> bandPtrsR = {
        rightBandBuffers_[0].data(), rightBandBuffers_[1].data(),
        rightBandBuffers_[2].data(), rightBandBuffers_[3].data()
    };

    // 1. Split Left and Right into 4 bands
    crossover_.splitChannel(0, left, bandPtrsL.data(), numSamples);
    crossover_.splitChannel(1, right, bandPtrsR.data(), numSamples);

    // 2. Solo / Mute state logic
    bool anySolo = false;
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        if (bandSettings_[b].solo) {
            anySolo = true;
            break;
        }
    }

    // 3. Process dynamics per band
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        if ((anySolo && !bandSettings_[b].solo) || bandSettings_[b].mute) {
            std::fill_n(bandPtrsL[b], numSamples, 0.0f);
            std::fill_n(bandPtrsR[b], numSamples, 0.0f);
            continue;
        }

        compressors_[0][b].process(bandPtrsL[b], numSamples);
        compressors_[1][b].process(bandPtrsR[b], numSamples);
    }

    // 4. Sum bands back together into master output
    CrossoverFilterbank::sumChannel(bandPtrsL.data(), left, numSamples);
    CrossoverFilterbank::sumChannel(bandPtrsR.data(), right, numSamples);
}

float MultibandDynamics::getGainReductionDb(size_t bandIndex, size_t channel) const noexcept {
    if (bandIndex >= NUM_CROSSOVER_BANDS || channel >= 2) {
        return 0.0f;
    }
    return compressors_[channel][bandIndex].getGainReductionDb();
}

} // namespace strata::dsp
