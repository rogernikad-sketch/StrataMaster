#include "StereoProcessor.h"
#include <algorithm>

namespace strata::dsp {

StereoProcessor::StereoProcessor() {
    haasBuffer_.assign(2048, 0.0f);
}

void StereoProcessor::prepare(double sampleRate, size_t /*maxBlockSize*/) noexcept {
    sampleRate_ = sampleRate;
    sideHPF_.prepare(sampleRate);
    sideHPF_.setParameters(FilterType::HighPass, monoMakerFreq_, 0.0f, 0.7071f, true);

    haasBuffer_.assign(2048, 0.0f);
    haasWritePos_ = 0;
    haasDelaySamples_ = static_cast<size_t>(0.0025 * sampleRate); // 2.5 ms
}

void StereoProcessor::reset() noexcept {
    sideHPF_.reset();
    std::fill(haasBuffer_.begin(), haasBuffer_.end(), 0.0f);
    haasWritePos_ = 0;
}

void StereoProcessor::setMonoMakerFrequency(float freqHz) noexcept {
    monoMakerFreq_ = std::clamp(freqHz, 20.0f, 500.0f);
    sideHPF_.setParameters(FilterType::HighPass, monoMakerFreq_, 0.0f, 0.7071f, true);
}

void StereoProcessor::encodeMidSide(float* left, float* right, size_t numSamples) noexcept {
    for (size_t i = 0; i < numSamples; ++i) {
        const float l = left[i];
        const float r = right[i];
        left[i]  = (l + r) * INV_SQRT_TWO; // Mid
        right[i] = (l - r) * INV_SQRT_TWO; // Side
    }
}

void StereoProcessor::decodeMidSide(float* mid, float* side, size_t numSamples) noexcept {
    for (size_t i = 0; i < numSamples; ++i) {
        const float m = mid[i];
        const float s = side[i];
        mid[i]  = (m + s) * INV_SQRT_TWO; // Left
        side[i] = (m - s) * INV_SQRT_TWO; // Right
    }
}

void StereoProcessor::applyWidth(float* left, float* right, size_t numSamples, float width) noexcept {
    encodeMidSide(left, right, numSamples);
    for (size_t i = 0; i < numSamples; ++i) {
        right[i] *= width; // Scale Side channel
    }
    decodeMidSide(left, right, numSamples);
}

void StereoProcessor::process(float* left, float* right, size_t numSamples, StereoMode mode, float width) noexcept {
    switch (mode) {
        case StereoMode::Stereo: {
            if (monoMakerEnabled_ || stereoizerEnabled_ || width != 1.0f) {
                encodeMidSide(left, right, numSamples);

                // 1. Mono-maker: High-pass the Side channel
                if (monoMakerEnabled_) {
                    sideHPF_.processBlock(right, numSamples);
                }

                // 2. Width scaling on Side
                if (width != 1.0f) {
                    for (size_t i = 0; i < numSamples; ++i) {
                        right[i] *= width;
                    }
                }

                // 3. Stereoizer / Haas decorrelation (Side injection from Mid)
                if (stereoizerEnabled_ && stereoizerAmount_ > 0.001f) {
                    const size_t bufSize = haasBuffer_.size();
                    const float amount = stereoizerAmount_ * 0.35f;

                    for (size_t i = 0; i < numSamples; ++i) {
                        haasBuffer_[haasWritePos_] = left[i];
                        const size_t readPos = (haasWritePos_ + bufSize - haasDelaySamples_) % bufSize;
                        const float delayedMid = haasBuffer_[readPos];
                        right[i] += amount * delayedMid;
                        haasWritePos_ = (haasWritePos_ + 1) % bufSize;
                    }
                }

                decodeMidSide(left, right, numSamples);
            }
            break;
        }

        case StereoMode::MidSide: {
            encodeMidSide(left, right, numSamples);
            if (monoMakerEnabled_) {
                sideHPF_.processBlock(right, numSamples);
            }
            break;
        }

        case StereoMode::MidSolo: {
            encodeMidSide(left, right, numSamples);
            std::fill_n(right, numSamples, 0.0f); // Silence Side
            decodeMidSide(left, right, numSamples);
            break;
        }

        case StereoMode::SideSolo: {
            encodeMidSide(left, right, numSamples);
            if (monoMakerEnabled_) {
                sideHPF_.processBlock(right, numSamples);
            }
            std::fill_n(left, numSamples, 0.0f); // Silence Mid
            decodeMidSide(left, right, numSamples);
            break;
        }
    }
}

void StereoProcessor::applyMultibandWidth(float* bandBuffers[NUM_CROSSOVER_BANDS][2],
                                        size_t numSamples,
                                        const std::array<float, NUM_CROSSOVER_BANDS>& widths) noexcept {
    for (size_t b = 0; b < NUM_CROSSOVER_BANDS; ++b) {
        if (std::abs(widths[b] - 1.0f) > 0.001f) {
            applyWidth(bandBuffers[b][0], bandBuffers[b][1], numSamples, widths[b]);
        }
    }
}

} // namespace strata::dsp
