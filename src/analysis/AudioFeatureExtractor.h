#pragma once

#include <vector>
#include <atomic>
#include <cstddef>
#include <cmath>

namespace strata::analysis {

struct AudioFeatures {
    float peakDb{ -100.0f };
    float rmsDb{ -100.0f };
    float crestFactorDb{ 0.0f };
    float spectralCentroidHz{ 1000.0f };
    float spectralTiltDb{ 0.0f }; // Low vs High energy ratio in dB
    float stereoWidthRatio{ 1.0f }; // Side RMS / Mid RMS
};

/**
 * Real-time Audio Feature Extractor.
 * Computes Crest Factor, Spectral Centroid, Spectral Tilt, and Stereo Ratio.
 */
class AudioFeatureExtractor final {
public:
    AudioFeatureExtractor();
    ~AudioFeatureExtractor() = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void processStereo(const float* left, const float* right, size_t numSamples) noexcept;

    [[nodiscard]] AudioFeatures getFeatures() const noexcept;

private:
    double sampleRate_{ 44100.0 };

    float peak_{ 0.0f };
    double sumSq_{ 0.0 };
    double sumMidSq_{ 0.0 };
    double sumSideSq_{ 0.0 };
    size_t sampleCount_{ 0 };

    std::atomic<float> crestFactorDb_{ 0.0f };
    std::atomic<float> rmsDb_{ -100.0f };
    std::atomic<float> peakDb_{ -100.0f };
    std::atomic<float> stereoWidthRatio_{ 1.0f };
    std::atomic<float> spectralCentroidHz_{ 1000.0f };
    std::atomic<float> spectralTiltDb_{ 0.0f };
};

} // namespace strata::analysis
