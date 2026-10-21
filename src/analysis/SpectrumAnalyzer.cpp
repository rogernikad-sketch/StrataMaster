#include "SpectrumAnalyzer.h"
#include "../dsp/common/DecibelUtils.h"
#include <algorithm>
#include <numbers>

namespace strata::analysis {

SpectrumAnalyzer::SpectrumAnalyzer() {
    prepare(44100.0);
}

void SpectrumAnalyzer::prepare(double sampleRate) noexcept {
    sampleRate_ = sampleRate;

    // 1. Hann Window
    hannWindow_.resize(FFT_SIZE);
    constexpr double twoPi = 2.0 * std::numbers::pi_v<double>;
    for (size_t n = 0; n < FFT_SIZE; ++n) {
        hannWindow_[n] = static_cast<float>(0.5 * (1.0 - std::cos(twoPi * static_cast<double>(n) / static_cast<double>(FFT_SIZE - 1))));
    }

    // 2. Precompute Radix-2 Bit-Reversal Indices for 4096 points (12 bits)
    bitReversedIndices_.resize(FFT_SIZE);
    constexpr size_t numBits = 12; // 2^12 = 4096
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        size_t rev = 0;
        size_t temp = i;
        for (size_t b = 0; b < numBits; ++b) {
            rev = (rev << 1) | (temp & 1);
            temp >>= 1;
        }
        bitReversedIndices_[i] = rev;
    }

    // 3. Precompute Twiddle Factors
    twiddles_.resize(FFT_SIZE / 2);
    for (size_t k = 0; k < FFT_SIZE / 2; ++k) {
        const double angle = -twoPi * static_cast<double>(k) / static_cast<double>(FFT_SIZE);
        twiddles_[k] = std::complex<float>(static_cast<float>(std::cos(angle)),
                                           static_cast<float>(std::sin(angle)));
    }

    inputAccumulator_.assign(FFT_SIZE, 0.0f);
    writePos_ = 0;

    latestMagnitudesDb_.assign(NUM_BINS, -100.0f);
    fftBuffer_.assign(FFT_SIZE, std::complex<float>(0.0f, 0.0f));
    newFrameReady_.store(false, std::memory_order_relaxed);
}

void SpectrumAnalyzer::reset() noexcept {
    std::fill(inputAccumulator_.begin(), inputAccumulator_.end(), 0.0f);
    writePos_ = 0;
    std::fill(latestMagnitudesDb_.begin(), latestMagnitudesDb_.end(), -100.0f);
    newFrameReady_.store(false, std::memory_order_relaxed);
}

void SpectrumAnalyzer::performFFT(std::vector<std::complex<float>>& data) const noexcept {
    // 1. Bit-reversal permutation in-place
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        const size_t j = bitReversedIndices_[i];
        if (i < j) {
            std::swap(data[i], data[j]);
        }
    }

    // 2. Cooley-Tukey Radix-2 Butterflies
    for (size_t len = 2; len <= FFT_SIZE; len <<= 1) {
        const size_t half = len >> 1;
        const size_t step = FFT_SIZE / len;

        for (size_t i = 0; i < FFT_SIZE; i += len) {
            for (size_t j = 0; j < half; ++j) {
                const auto twiddle = twiddles_[j * step];
                const auto u = data[i + j];
                const auto v = data[i + j + half] * twiddle;
                data[i + j]        = u + v;
                data[i + j + half] = u - v;
            }
        }
    }
}

void SpectrumAnalyzer::processDirectFFT(const float* input, std::vector<float>& outputMagsDb) noexcept {
    if (fftBuffer_.size() != FFT_SIZE) {
        fftBuffer_.resize(FFT_SIZE);
    }
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        fftBuffer_[i] = std::complex<float>(input[i] * hannWindow_[i], 0.0f);
    }

    performFFT(fftBuffer_);

    if (outputMagsDb.size() != NUM_BINS) {
        outputMagsDb.resize(NUM_BINS);
    }
    constexpr float norm = 2.0f / static_cast<float>(FFT_SIZE);

    for (size_t k = 0; k < NUM_BINS; ++k) {
        const float mag = std::abs(fftBuffer_[k]) * norm * 2.0f; // *2 accounts for Hann window loss
        outputMagsDb[k] = dsp::gainToDecibels(std::max(1.0e-5f, mag));
    }
}

void SpectrumAnalyzer::pushAudioBlock(const float* left, const float* right, size_t numSamples) noexcept {
    for (size_t i = 0; i < numSamples; ++i) {
        // Average Left + Right for mono RTA display
        const float mono = 0.5f * (left[i] + right[i]);
        inputAccumulator_[writePos_++] = mono;

        if (writePos_ >= FFT_SIZE) {
            processDirectFFT(inputAccumulator_.data(), latestMagnitudesDb_);
            newFrameReady_.store(true, std::memory_order_relaxed);

            // 50% overlap step: keep last 2048 samples
            std::copy(inputAccumulator_.begin() + 2048, inputAccumulator_.end(), inputAccumulator_.begin());
            writePos_ = 2048;
        }
    }
}

bool SpectrumAnalyzer::getLatestMagnitudes(std::vector<float>& destMags, SpectrumSmoothing smoothing) noexcept {
    if (!newFrameReady_.load(std::memory_order_relaxed)) {
        return false;
    }

    destMags.resize(NUM_BINS);
    if (smoothing == SpectrumSmoothing::None) {
        std::copy(latestMagnitudesDb_.begin(), latestMagnitudesDb_.end(), destMags.begin());
    } else {
        applySmoothing(latestMagnitudesDb_, destMags, smoothing);
    }

    newFrameReady_.store(false, std::memory_order_relaxed);
    return true;
}

void SpectrumAnalyzer::applySmoothing(const std::vector<float>& rawDb, std::vector<float>& smoothedDb, SpectrumSmoothing smoothing) const noexcept {
    const float factor = (smoothing == SpectrumSmoothing::Octave1_3) ? 0.1156f : 0.0577f; // (2^(1/6)-1) and (2^(1/12)-1)

    for (size_t k = 0; k < NUM_BINS; ++k) {
        if (k < 4) {
            smoothedDb[k] = rawDb[k];
            continue;
        }

        const size_t span = std::max(size_t{1}, static_cast<size_t>(static_cast<float>(k) * factor));
        const size_t start = (k >= span) ? (k - span) : 0;
        const size_t end = std::min(NUM_BINS - 1, k + span);

        float sum = 0.0f;
        for (size_t j = start; j <= end; ++j) {
            sum += rawDb[j];
        }
        smoothedDb[k] = sum / static_cast<float>(end - start + 1);
    }
}

} // namespace strata::analysis
