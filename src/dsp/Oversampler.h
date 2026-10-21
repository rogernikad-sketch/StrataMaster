#pragma once

#include <vector>
#include <array>
#include <cstddef>
#include <cmath>

namespace strata::dsp {

enum class OversamplingFactor {
    None = 1,
    X2   = 2,
    X4   = 4,
    X8   = 8
};

/**
 * Polyphase half-band FIR filter stage for 2x upsampling and 2x downsampling.
 * Designed with Kaiser-windowed sinc coefficients providing > 90 dB stopband attenuation
 * and strictly linear phase (constant group delay).
 */
class HalfbandStage {
public:
    HalfbandStage() = default;

    /**
     * Prepares filter with order parameter M (half-length).
     * Filter length is N = 2*M + 1.
     * M must be an even integer (e.g. 30 or 16).
     * Group delay is M/2 samples at the lower sample rate.
     */
    void prepare(size_t M, size_t maxBlockSize);
    void reset() noexcept;

    /**
     * Up-samples input signal from rate Fs to 2*Fs.
     * in: length numInputSamples
     * out: length 2 * numInputSamples
     */
    void upsample(const float* in, float* out, size_t numInputSamples) noexcept;

    /**
     * Down-samples input signal from rate 2*Fs to Fs.
     * in: length 2 * numOutputSamples
     * out: length numOutputSamples
     */
    void downsample(const float* in, float* out, size_t numOutputSamples) noexcept;

    [[nodiscard]] size_t getLatency() const noexcept { return M_; }

private:
    void computeCoefficients(size_t M);

    size_t M_{ 48 }; // Half-length (even integer)
    std::vector<float> polyCoeffs_; // Unique odd coefficients (length M/2)

    // History buffers for stereo channels (per stage single channel instance)
    std::vector<float> upHistory_;
    std::vector<float> downHistoryOdd_;
    std::vector<float> downHistoryEven_;
    size_t upPos_{ 0 };
    size_t downOddPos_{ 0 };
    size_t downEvenPos_{ 0 };
};

/**
 * Multi-stage polyphase half-band FIR Oversampling Engine.
 * Supports 1x (bypass), 2x, 4x, and 8x oversampling with > 90 dBFS alias rejection.
 * Guarantees zero memory allocation on the real-time audio thread.
 */
class Oversampler {
public:
    Oversampler();
    ~Oversampler() = default;

    void prepare(double sampleRate, size_t maxBlockSize, OversamplingFactor factor = OversamplingFactor::X4);
    void reset() noexcept;

    void setFactor(OversamplingFactor factor);
    [[nodiscard]] OversamplingFactor getFactor() const noexcept { return factor_; }
    [[nodiscard]] size_t getLatencyInSamples() const noexcept;
    [[nodiscard]] double getOversampledRate() const noexcept;

    /**
     * Up-samples a 2-channel stereo buffer.
     * inputChannels: array of 2 pointers [L, R], each containing numInputSamples
     * oversampledOutputs: output array of 2 pointers [L, R] pointing to internal oversampled memory
     * numOversampledSamples: output number of samples (numInputSamples * factor)
     */
    void upsample(const float* const* inputChannels, size_t numInputSamples,
                  float* const*& oversampledOutputs, size_t& numOversampledSamples) noexcept;

    /**
     * Down-samples internal oversampled memory (processed in-place) back to output stereo buffers.
     * outputChannels: array of 2 pointers [L, R], each receiving numOutputSamples
     */
    void downsample(float* const* outputChannels, size_t numOutputSamples) noexcept;

    /**
     * Returns pointer to dry delay line for delay-compensated wet/dry blending.
     */
    const float* getDryDelayedChannel(size_t channel) const noexcept {
        return (channel < 2) ? dryDelayBuffer_[channel].data() : nullptr;
    }

private:
    void updateDryDelay(const float* const* inputChannels, size_t numSamples) noexcept;

    double sampleRate_{ 44100.0 };
    size_t maxBlockSize_{ 1024 };
    OversamplingFactor factor_{ OversamplingFactor::X4 };

    // Cascaded stages:
    // Left channel: stage 1 (2x), stage 2 (4x), stage 3 (8x)
    // Right channel: stage 1 (2x), stage 2 (4x), stage 3 (8x)
    std::array<HalfbandStage, 3> stagesL_;
    std::array<HalfbandStage, 3> stagesR_;

    // Internal intermediate working buffers
    std::vector<float> stage1BufferL_;
    std::vector<float> stage1BufferR_;
    std::vector<float> stage2BufferL_;
    std::vector<float> stage2BufferR_;
    std::vector<float> stage3BufferL_;
    std::vector<float> stage3BufferR_;

    std::array<float*, 2> oversampledPtrs_{ nullptr, nullptr };

    // Dry delay compensation buffers
    std::array<std::vector<float>, 2> dryHistory_;
    std::array<std::vector<float>, 2> dryDelayBuffer_;
    size_t dryWritePos_{ 0 };
};

} // namespace strata::dsp
