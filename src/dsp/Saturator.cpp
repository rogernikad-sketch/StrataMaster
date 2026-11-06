#include "Saturator.h"
#include <algorithm>
#include <cmath>

namespace strata::dsp {

Saturator::Saturator() {
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        bandSettings_[b] = BandExciterSettings{};
    }
}

void Saturator::prepare(double sampleRate, size_t maxBlockSize, OversamplingFactor factor) {
    sampleRate_ = sampleRate;
    maxBlockSize_ = maxBlockSize;

    widebandOversampler_.prepare(sampleRate, maxBlockSize, factor);
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        bandOversamplers_[b].prepare(sampleRate, maxBlockSize, factor);
    }

    const double oversampledRate = widebandOversampler_.getOversampledRate();
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        dcBlockers_[b][0].prepare(oversampledRate);
        dcBlockers_[b][1].prepare(oversampledRate);
    }
    widebandDCBlockers_[0].prepare(oversampledRate);
    widebandDCBlockers_[1].prepare(oversampledRate);

    workBufferL_.assign(maxBlockSize, 0.0f);
    workBufferR_.assign(maxBlockSize, 0.0f);

    reset();
}

void Saturator::reset() noexcept {
    widebandOversampler_.reset();
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        bandOversamplers_[b].reset();
        dcBlockers_[b][0].reset();
        dcBlockers_[b][1].reset();
    }
    widebandDCBlockers_[0].reset();
    widebandDCBlockers_[1].reset();
}

void Saturator::setOversamplingFactor(OversamplingFactor factor) {
    widebandOversampler_.setFactor(factor);
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        bandOversamplers_[b].setFactor(factor);
    }

    const double oversampledRate = widebandOversampler_.getOversampledRate();
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        dcBlockers_[b][0].prepare(oversampledRate);
        dcBlockers_[b][1].prepare(oversampledRate);
    }
    widebandDCBlockers_[0].prepare(oversampledRate);
    widebandDCBlockers_[1].prepare(oversampledRate);
}

void Saturator::setBandSettings(size_t band, const BandExciterSettings& settings) noexcept {
    if (band < NUM_CROSSOVER_BANDS) {
        bandSettings_[band] = settings;
    }
}

const BandExciterSettings& Saturator::getBandSettings(size_t band) const noexcept {
    static const BandExciterSettings defaultSettings{};
    return (band < NUM_CROSSOVER_BANDS) ? bandSettings_[band] : defaultSettings;
}

float Saturator::shapeTape(float x, float drive) noexcept {
    const float k = 1.0f + 10.0f * drive;
    const float u = x * k;
    const float y = std::tanh(u);
    const float comp = 1.0f / std::sqrt(1.0f + 0.5f * (k - 1.0f));
    return y * comp;
}

float Saturator::shapeTube(float x, float drive, DCBlocker& dc) noexcept {
    const float k = 1.0f + 12.0f * drive;
    const float u = x * k;
    const float absU = std::abs(u);
    // Asymmetric triode transfer characteristic generating strong 2nd-order harmonic
    const float y = u + 0.35f * (u * u) / (1.0f + 0.5f * absU) - 0.05f * (u * u * u) / (1.0f + 0.3f * u * u);
    const float yClean = dc.processSample(y);
    const float comp = 1.0f / std::sqrt(1.0f + 0.4f * (k - 1.0f));
    return yClean * comp;
}

float Saturator::shapeWarm(float x, float drive, DCBlocker& dc) noexcept {
    const float k = 1.0f + 8.0f * drive;
    const float u = x * k;
    const float absU = std::abs(u);
    // Even-order predominant transformer/tube warmth
    const float y = u + 0.22f * (u * u) / (1.0f + 0.6f * absU);
    const float yClean = dc.processSample(y);
    const float comp = 1.0f / std::sqrt(1.0f + 0.3f * (k - 1.0f));
    return yClean * comp;
}

float Saturator::shapeRetro(float x, float drive) noexcept {
    const float k = 1.0f + 14.0f * drive;
    const float u = x * k;
    float y = 0.0f;
    if (u > 1.2f) {
        y = 0.8f;
    } else if (u < -1.2f) {
        y = -0.8f;
    } else {
        y = u - (u * u * u) / 4.32f;
    }
    const float comp = 1.0f / std::sqrt(1.0f + 0.5f * (k - 1.0f));
    return y * comp;
}

void Saturator::processOversampledBand(float* left, float* right, size_t numSamples,
                                      SaturationMode mode, float drive, float mix,
                                      size_t bandIdx, Oversampler& os) noexcept {
    if (drive <= 0.001f && mix <= 0.001f) {
        // Flat wire, no-op
        return;
    }

    const float* inChannels[2] = { left, right };
    float* const* oversampledOutputs = nullptr;
    size_t numOversampled = 0;

    os.upsample(inChannels, numSamples, oversampledOutputs, numOversampled);

    auto* overL = oversampledOutputs[0];
    auto* overR = oversampledOutputs[1];

    auto& dcL = (bandIdx < NUM_CROSSOVER_BANDS) ? dcBlockers_[bandIdx][0] : widebandDCBlockers_[0];
    auto& dcR = (bandIdx < NUM_CROSSOVER_BANDS) ? dcBlockers_[bandIdx][1] : widebandDCBlockers_[1];

    switch (mode) {
        case SaturationMode::Tape: {
            for (size_t i = 0; i < numOversampled; ++i) {
                overL[i] = shapeTape(overL[i], drive);
                overR[i] = shapeTape(overR[i], drive);
            }
            break;
        }
        case SaturationMode::Tube: {
            for (size_t i = 0; i < numOversampled; ++i) {
                overL[i] = shapeTube(overL[i], drive, dcL);
                overR[i] = shapeTube(overR[i], drive, dcR);
            }
            break;
        }
        case SaturationMode::Warm: {
            for (size_t i = 0; i < numOversampled; ++i) {
                overL[i] = shapeWarm(overL[i], drive, dcL);
                overR[i] = shapeWarm(overR[i], drive, dcR);
            }
            break;
        }
        case SaturationMode::Retro: {
            for (size_t i = 0; i < numOversampled; ++i) {
                overL[i] = shapeRetro(overL[i], drive);
                overR[i] = shapeRetro(overR[i], drive);
            }
            break;
        }
    }

    // Downsample wet saturated signal
    float* outChannels[2] = { left, right };
    os.downsample(outChannels, numSamples);

    // Apply delay-compensated dry/wet blend
    if (mix < 0.999f) {
        const float* dryL = os.getDryDelayedChannel(0);
        const float* dryR = os.getDryDelayedChannel(1);
        const float wetGain = mix;
        const float dryGain = 1.0f - mix;

        for (size_t i = 0; i < numSamples; ++i) {
            left[i]  = dryGain * dryL[i] + wetGain * left[i];
            right[i] = dryGain * dryR[i] + wetGain * right[i];
        }
    }
}

void Saturator::processStereo(float* left, float* right, size_t numSamples,
                              SaturationMode mode, float drive, float mix) noexcept {
    processOversampledBand(left, right, numSamples, mode, drive, mix, NUM_CROSSOVER_BANDS, widebandOversampler_);
}

void Saturator::processMultiband(float* bandBuffers[NUM_CROSSOVER_BANDS][2], size_t numSamples) noexcept {
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        const auto& s = bandSettings_[b];
        if (!s.enabled) {
            continue;
        }

        processOversampledBand(bandBuffers[b][0], bandBuffers[b][1], numSamples,
                               s.mode, s.drive, s.mix, b, bandOversamplers_[b]);
    }
}

} // namespace strata::dsp
