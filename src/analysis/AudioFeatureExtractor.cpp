#include "AudioFeatureExtractor.h"
#include "../dsp/common/DecibelUtils.h"
#include <algorithm>

namespace strata::analysis {

AudioFeatureExtractor::AudioFeatureExtractor() {
    prepare(44100.0);
}

void AudioFeatureExtractor::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
}

void AudioFeatureExtractor::reset() noexcept {
    peak_ = 0.0f;
    sumSq_ = 0.0;
    sumMidSq_ = 0.0;
    sumSideSq_ = 0.0;
    sampleCount_ = 0;

    crestFactorDb_.store(0.0f, std::memory_order_relaxed);
    rmsDb_.store(-100.0f, std::memory_order_relaxed);
    peakDb_.store(-100.0f, std::memory_order_relaxed);
    stereoWidthRatio_.store(1.0f, std::memory_order_relaxed);
    spectralCentroidHz_.store(1000.0f, std::memory_order_relaxed);
    spectralTiltDb_.store(0.0f, std::memory_order_relaxed);
}

void AudioFeatureExtractor::processStereo(const float* left, const float* right, size_t numSamples) noexcept {
    for (size_t i = 0; i < numSamples; ++i) {
        const float l = left[i];
        const float r = right[i];

        const float absL = std::abs(l);
        const float absR = std::abs(r);
        peak_ = std::max(peak_, std::max(absL, absR));

        const float mono = 0.5f * (l + r);
        const float side = 0.5f * (l - r);

        sumSq_     += static_cast<double>(l * l + r * r) * 0.5;
        sumMidSq_  += static_cast<double>(mono * mono);
        sumSideSq_ += static_cast<double>(side * side);
        ++sampleCount_;
    }

    // Update statistics every 1024 samples
    if (sampleCount_ >= 1024) {
        const float rms = static_cast<float>(std::sqrt(sumSq_ / static_cast<double>(sampleCount_)));
        const float pDb = dsp::gainToDecibels(peak_);
        const float rDb = dsp::gainToDecibels(rms);

        peakDb_.store(pDb, std::memory_order_relaxed);
        rmsDb_.store(rDb, std::memory_order_relaxed);

        const float crest = (rms > 1.0e-5f) ? (pDb - rDb) : 0.0f;
        crestFactorDb_.store(crest, std::memory_order_relaxed);

        const double midRms = std::sqrt(sumMidSq_ / static_cast<double>(sampleCount_));
        const double sideRms = std::sqrt(sumSideSq_ / static_cast<double>(sampleCount_));
        const float widthRatio = (midRms > 1.0e-5) ? static_cast<float>(sideRms / midRms) : 1.0f;
        stereoWidthRatio_.store(widthRatio, std::memory_order_relaxed);

        // Decay peak slowly and reset window
        peak_ *= 0.95f;
        sumSq_ = 0.0;
        sumMidSq_ = 0.0;
        sumSideSq_ = 0.0;
        sampleCount_ = 0;
    }
}

AudioFeatures AudioFeatureExtractor::getFeatures() const noexcept {
    AudioFeatures f;
    f.peakDb = peakDb_.load(std::memory_order_relaxed);
    f.rmsDb = rmsDb_.load(std::memory_order_relaxed);
    f.crestFactorDb = crestFactorDb_.load(std::memory_order_relaxed);
    f.stereoWidthRatio = stereoWidthRatio_.load(std::memory_order_relaxed);
    f.spectralCentroidHz = spectralCentroidHz_.load(std::memory_order_relaxed);
    f.spectralTiltDb = spectralTiltDb_.load(std::memory_order_relaxed);
    return f;
}

} // namespace strata::analysis
