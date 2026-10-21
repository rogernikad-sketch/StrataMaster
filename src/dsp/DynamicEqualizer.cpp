#include "DynamicEqualizer.h"
#include "common/DecibelUtils.h"
#include <cmath>

namespace strata::dsp {

// ==============================================================================
// DynamicBand Implementation
// ==============================================================================

void DynamicBand::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
    audioFilter_.prepare(sampleRate);
    sidechainFilter_.prepare(sampleRate);
    reset();
}

void DynamicBand::reset() noexcept {
    audioFilter_.reset();
    sidechainFilter_.reset();
    envelopeLevel_ = 0.0f;
    dynamicOffsetDb_ = 0.0f;
    gainReductionDb_.store(0.0f, std::memory_order_relaxed);
    updateEnvelopeBallistics();
    sidechainFilter_.snapToTarget();
    audioFilter_.snapToTarget();
}

void DynamicBand::updateEnvelopeBallistics() noexcept {
    if (sampleRate_ <= 0.0) {
        return;
    }

    const float attSec = std::max(0.0005f, settings_.attackMs * 0.001f);
    const float relSec = std::max(0.005f, settings_.releaseMs * 0.001f);

    attackCoeff_  = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate_ * attSec)));
    releaseCoeff_ = static_cast<float>(1.0 - std::exp(-1.0 / (sampleRate_ * relSec)));
}

void DynamicBand::updateSettings(const DynamicBandSettings& settings, bool snapImmediately) noexcept {
    if (!snapImmediately &&
        settings_.enabled == settings.enabled &&
        settings_.type == settings.type &&
        std::abs(settings_.frequencyHz - settings.frequencyHz) < 0.01f &&
        std::abs(settings_.gainDb - settings.gainDb) < 0.01f &&
        std::abs(settings_.q - settings.q) < 0.001f &&
        settings_.dynamicEnabled == settings.dynamicEnabled &&
        std::abs(settings_.thresholdDb - settings.thresholdDb) < 0.01f &&
        std::abs(settings_.dynamicRangeDb - settings.dynamicRangeDb) < 0.01f &&
        std::abs(settings_.attackMs - settings.attackMs) < 0.1f &&
        std::abs(settings_.releaseMs - settings.releaseMs) < 0.1f) {
        return;
    }

    settings_ = settings;
    updateEnvelopeBallistics();

    // Prepare sidechain detector filter (bandpass at center frequency)
    sidechainFilter_.setParameters(FilterType::BandPass, settings_.frequencyHz, 0.0f, settings_.q, snapImmediately);

    if (!settings_.dynamicEnabled) {
        dynamicOffsetDb_ = 0.0f;
        gainReductionDb_.store(0.0f, std::memory_order_relaxed);
        audioFilter_.setParameters(settings_.type, settings_.frequencyHz, settings_.gainDb, settings_.q, snapImmediately);
    } else {
        // Also initialize audio filter at base settings
        audioFilter_.setParameters(settings_.type, settings_.frequencyHz, settings_.gainDb, settings_.q, snapImmediately);
    }
}

float DynamicBand::processSample(float input) noexcept {
    if (!settings_.enabled) {
        return input;
    }

    if (!settings_.dynamicEnabled) {
        return audioFilter_.processSample(input);
    }

    // Dynamic processing
    const float scSample = sidechainFilter_.processSample(input);
    const float rectified = std::abs(scSample);

    if (rectified > envelopeLevel_) {
        envelopeLevel_ += attackCoeff_ * (rectified - envelopeLevel_);
    } else {
        envelopeLevel_ += releaseCoeff_ * (rectified - envelopeLevel_);
    }

    const float envDb = gainToDecibels(envelopeLevel_, -96.0f);
    if (envDb > settings_.thresholdDb) {
        const float overshoot = envDb - settings_.thresholdDb;
        const float normOvershoot = std::clamp(overshoot / 18.0f, 0.0f, 1.0f);
        dynamicOffsetDb_ = settings_.dynamicRangeDb * normOvershoot;
    } else {
        dynamicOffsetDb_ = 0.0f;
    }

    gainReductionDb_.store(dynamicOffsetDb_, std::memory_order_relaxed);

    const float effGainDb = settings_.gainDb + dynamicOffsetDb_;
    audioFilter_.setParameters(settings_.type, settings_.frequencyHz, effGainDb, settings_.q);

    return audioFilter_.processSample(input);
}

void DynamicBand::processBlock(float* buffer, size_t numSamples) noexcept {
    if (!settings_.enabled || numSamples == 0) {
        return;
    }

    if (!settings_.dynamicEnabled) {
        audioFilter_.processBlock(buffer, numSamples);
        return;
    }

    // Sub-block processing (every 16 samples) for optimum dynamic efficiency
    constexpr size_t subBlockSize = 16;
    size_t samplesProcessed = 0;

    while (samplesProcessed < numSamples) {
        const size_t chunkSize = std::min(subBlockSize, numSamples - samplesProcessed);
        float* chunk = buffer + samplesProcessed;

        // 1. Detect envelope over chunk
        for (size_t i = 0; i < chunkSize; ++i) {
            const float sc = sidechainFilter_.processSample(chunk[i]);
            const float rectified = std::abs(sc);
            if (rectified > envelopeLevel_) {
                envelopeLevel_ += attackCoeff_ * (rectified - envelopeLevel_);
            } else {
                envelopeLevel_ += releaseCoeff_ * (rectified - envelopeLevel_);
            }
        }

        // 2. Compute dynamic modulation
        const float envDb = gainToDecibels(envelopeLevel_, -96.0f);
        if (envDb > settings_.thresholdDb) {
            const float overshoot = envDb - settings_.thresholdDb;
            const float normOvershoot = std::clamp(overshoot / 18.0f, 0.0f, 1.0f);
            dynamicOffsetDb_ = settings_.dynamicRangeDb * normOvershoot;
        } else {
            dynamicOffsetDb_ = 0.0f;
        }

        gainReductionDb_.store(dynamicOffsetDb_, std::memory_order_relaxed);

        // 3. Update coefficients for this chunk with smooth interpolation
        const float effGainDb = settings_.gainDb + dynamicOffsetDb_;
        audioFilter_.setParameters(settings_.type, settings_.frequencyHz, effGainDb, settings_.q);

        // 4. Process audio chunk
        for (size_t i = 0; i < chunkSize; ++i) {
            chunk[i] = audioFilter_.processSample(chunk[i]);
        }

        samplesProcessed += chunkSize;
    }
}

// ==============================================================================
// DynamicEqualizer Implementation
// ==============================================================================

void DynamicEqualizer::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;

    // Default 8-band mastering curve
    const std::array<float, NUM_EQ_BANDS> defaultFreqs = {
        30.0f, 80.0f, 250.0f, 800.0f, 2000.0f, 5000.0f, 10000.0f, 16000.0f
    };
    const std::array<FilterType, NUM_EQ_BANDS> defaultTypes = {
        FilterType::HighPass, FilterType::LowShelf, FilterType::Bell, FilterType::Bell,
        FilterType::Bell,     FilterType::Bell,     FilterType::HighShelf, FilterType::LowPass
    };

    for (size_t b = 0; b < NUM_EQ_BANDS; ++b) {
        bandSettings_[b].enabled = (b > 0 && b < 7); // Default bands 1-6 active, HP/LP off
        bandSettings_[b].frequencyHz = defaultFreqs[b];
        bandSettings_[b].type = defaultTypes[b];
        bandSettings_[b].gainDb = 0.0f;
        bandSettings_[b].q = 1.0f;
        bandSettings_[b].dynamicEnabled = false;
        bandSettings_[b].thresholdDb = -20.0f;
        bandSettings_[b].dynamicRangeDb = -6.0f;
        bandSettings_[b].attackMs = 10.0f;
        bandSettings_[b].releaseMs = 100.0f;

        for (size_t ch = 0; ch < 2; ++ch) {
            bands_[ch][b].prepare(sampleRate);
            bands_[ch][b].updateSettings(bandSettings_[b], true);
        }
    }
}

void DynamicEqualizer::reset() noexcept {
    for (size_t ch = 0; ch < 2; ++ch) {
        for (size_t b = 0; b < NUM_EQ_BANDS; ++b) {
            bands_[ch][b].reset();
        }
    }
}

void DynamicEqualizer::setBandSettings(size_t bandIndex, const DynamicBandSettings& settings, bool snapImmediately) noexcept {
    if (bandIndex >= NUM_EQ_BANDS) {
        return;
    }
    bandSettings_[bandIndex] = settings;
    bands_[0][bandIndex].updateSettings(settings, snapImmediately);
    bands_[1][bandIndex].updateSettings(settings, snapImmediately);
}

const DynamicBandSettings& DynamicEqualizer::getBandSettings(size_t bandIndex) const noexcept {
    static const DynamicBandSettings fallback;
    if (bandIndex >= NUM_EQ_BANDS) {
        return fallback;
    }
    return bandSettings_[bandIndex];
}

void DynamicEqualizer::processStereo(float* left, float* right, size_t numSamples) noexcept {
    for (size_t b = 0; b < NUM_EQ_BANDS; ++b) {
        if (!bandSettings_[b].enabled) {
            continue;
        }
        bands_[0][b].processBlock(left, numSamples);
        bands_[1][b].processBlock(right, numSamples);
    }
}

float DynamicEqualizer::getGainReductionDb(size_t bandIndex, size_t channel) const noexcept {
    if (bandIndex >= NUM_EQ_BANDS || channel >= 2) {
        return 0.0f;
    }
    return bands_[channel][bandIndex].getGainReductionDb();
}

double DynamicEqualizer::getCompositeMagnitudeResponseDb(double frequencyHz) const noexcept {
    double totalDb = 0.0;
    for (size_t b = 0; b < NUM_EQ_BANDS; ++b) {
        if (bandSettings_[b].enabled) {
            totalDb += bands_[0][b].getMagnitudeResponseDb(frequencyHz);
        }
    }
    return totalDb;
}

} // namespace strata::dsp
