#pragma once

#include <vector>
#include <complex>
#include <atomic>
#include <cstddef>
#include <cmath>

namespace strata::analysis {

enum class SpectrumSmoothing {
    None,
    Octave1_3,
    Octave1_6
};

/**
 * 4096-point Real-Time FFT Spectrum Analyzer.
 * Features:
 * - 4096-point Radix-2 Cooley-Tukey decimation-in-time FFT (pure C++20)
 * - Hann windowing
 * - Logarithmic frequency binning and 1/3, 1/6 octave smoothing
 * - Lock-free FIFO frame pushing to UI thread
 */
class SpectrumAnalyzer final {
public:
    static constexpr size_t FFT_SIZE = 4096;
    static constexpr size_t NUM_BINS = FFT_SIZE / 2;

    SpectrumAnalyzer();
    ~SpectrumAnalyzer() = default;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    /** Pushes mono/stereo audio samples into accumulation buffer */
    void pushAudioBlock(const float* left, const float* right, size_t numSamples) noexcept;

    /** Returns true if a new FFT frame is ready; copies magnitudes (in dBFS) into dest array */
    bool getLatestMagnitudes(std::vector<float>& destMags, SpectrumSmoothing smoothing = SpectrumSmoothing::Octave1_3) noexcept;

    /** Directly processes an arbitrary buffer of FFT_SIZE for testing/offline analysis */
    void processDirectFFT(const float* input, std::vector<float>& outputMagsDb) noexcept;

    [[nodiscard]] double getBinFrequency(size_t binIndex) const noexcept {
        return static_cast<double>(binIndex) * sampleRate_ / static_cast<double>(FFT_SIZE);
    }

private:
    void performFFT(std::vector<std::complex<float>>& data) const noexcept;
    void applySmoothing(const std::vector<float>& rawDb, std::vector<float>& smoothedDb, SpectrumSmoothing smoothing) const noexcept;

    double sampleRate_{ 44100.0 };

    // Hann window
    std::vector<float> hannWindow_;

    // Twiddle factors for Radix-2 FFT
    std::vector<std::complex<float>> twiddles_;
    std::vector<size_t> bitReversedIndices_;

    // Sample accumulation buffer
    std::vector<float> inputAccumulator_;
    size_t writePos_{ 0 };

    // UI Frame Exchange (triple buffer / lock-free atomic flag)
    std::vector<float> latestMagnitudesDb_;
    std::atomic<bool> newFrameReady_{ false };

    // Preallocated buffer for FFT processing (zero runtime allocations)
    mutable std::vector<std::complex<float>> fftBuffer_;
};

} // namespace strata::analysis
