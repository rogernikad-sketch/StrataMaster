#include "MaseratiProcessor.h"
#include <cmath>
#include <algorithm>

namespace strata::dsp {

MaseratiProcessor::MaseratiProcessor() {
    reset();
}

void MaseratiProcessor::prepare(double sampleRate, size_t /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    for (int i = 0; i < 2; ++i) {
        bassFilterL_[i].prepare(sampleRate);
        bassFilterR_[i].prepare(sampleRate);
        trebleFilterL_[i].prepare(sampleRate);
        trebleFilterR_[i].prepare(sampleRate);
    }
    reset();
    updateFilters();
}

void MaseratiProcessor::reset() noexcept {
    envDetector_ = 0.0f;
    activityMeter_.store(0.0f, std::memory_order_relaxed);

    for (int i = 0; i < 2; ++i) {
        bassFilterL_[i].reset();
        bassFilterR_[i].reset();
        trebleFilterL_[i].reset();
        trebleFilterR_[i].reset();
    }
}

void MaseratiProcessor::setBass(float bassDb) noexcept {
    bassDb_ = std::clamp(bassDb, -12.0f, 12.0f);
    updateFilters();
}

void MaseratiProcessor::setTreble(float trebleDb) noexcept {
    trebleDb_ = std::clamp(trebleDb, -12.0f, 12.0f);
    updateFilters();
}

void MaseratiProcessor::setSensitivity(float sensitivityDb) noexcept {
    sensitivityDb_ = std::clamp(sensitivityDb, -24.0f, 12.0f);
}

void MaseratiProcessor::setEnabled(bool enabled) noexcept {
    enabled_ = enabled;
}

void MaseratiProcessor::updateFilters() noexcept {
    // Bass stage 1: 60Hz punch bell (punch foundation)
    const float bassPunchGain = bassDb_ * 0.65f;
    bassFilterL_[0].setParameters(FilterType::Bell, 60.0f, bassPunchGain, 1.4f, true);
    bassFilterR_[0].setParameters(FilterType::Bell, 60.0f, bassPunchGain, 1.4f, true);

    // Bass stage 2: 110Hz low-shelf (warmth & body)
    const float bassShelfGain = bassDb_ * 0.45f;
    bassFilterL_[1].setParameters(FilterType::LowShelf, 110.0f, bassShelfGain, 0.707f, true);
    bassFilterR_[1].setParameters(FilterType::LowShelf, 110.0f, bassShelfGain, 0.707f, true);

    // Treble stage 1: 3.8kHz presence bell (vocal & transient clarity)
    const float treblePresenceGain = trebleDb_ * 0.35f;
    trebleFilterL_[0].setParameters(FilterType::Bell, 3800.0f, treblePresenceGain, 0.9f, true);
    trebleFilterR_[0].setParameters(FilterType::Bell, 3800.0f, treblePresenceGain, 0.9f, true);

    // Treble stage 2: 12.5kHz air shelf (silky top-end sheen)
    const float trebleAirGain = trebleDb_ * 0.75f;
    trebleFilterL_[1].setParameters(FilterType::HighShelf, 12500.0f, trebleAirGain, 0.707f, true);
    trebleFilterR_[1].setParameters(FilterType::HighShelf, 12500.0f, trebleAirGain, 0.707f, true);
}

void MaseratiProcessor::processStereo(float* left, float* right, size_t numSamples) noexcept {
    if (!enabled_ || numSamples == 0) {
        activityMeter_.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const float sensGain = std::pow(10.0f, sensitivityDb_ / 20.0f);
    const float attackCoeff = std::exp(-1.0f / (0.005f * static_cast<float>(sampleRate_)));
    const float releaseCoeff = std::exp(-1.0f / (0.080f * static_cast<float>(sampleRate_)));

    float peakActivity = 0.0f;

    for (size_t i = 0; i < numSamples; ++i) {
        float l = left[i];
        float r = right[i];

        // 1. Dual-stage Bass Sculpting
        l = bassFilterL_[0].processSample(l);
        l = bassFilterL_[1].processSample(l);
        r = bassFilterR_[0].processSample(r);
        r = bassFilterR_[1].processSample(r);

        // 2. Dual-stage Treble Sculpting
        l = trebleFilterL_[0].processSample(l);
        l = trebleFilterL_[1].processSample(l);
        r = trebleFilterR_[0].processSample(r);
        r = trebleFilterR_[1].processSample(r);

        // 3. Sensitivity Dynamic Harmonic Drive
        const float sampleMag = (std::abs(l) + std::abs(r)) * 0.5f * sensGain;
        if (sampleMag > envDetector_) {
            envDetector_ = attackCoeff * envDetector_ + (1.0f - attackCoeff) * sampleMag;
        } else {
            envDetector_ = releaseCoeff * envDetector_ + (1.0f - releaseCoeff) * sampleMag;
        }

        peakActivity = std::max(peakActivity, envDetector_);

        // Subtle analog saturation proportional to sensitivity drive
        if (sensGain > 1.0f || envDetector_ > 0.4f) {
            const float satDrive = std::clamp(envDetector_ * 0.25f, 0.0f, 0.5f);
            l = std::tanh(l * (1.0f + satDrive)) / (1.0f + 0.3f * satDrive);
            r = std::tanh(r * (1.0f + satDrive)) / (1.0f + 0.3f * satDrive);
        }

        left[i] = l;
        right[i] = r;
    }

    activityMeter_.store(std::clamp(peakActivity, 0.0f, 1.5f), std::memory_order_relaxed);
}

} // namespace strata::dsp
