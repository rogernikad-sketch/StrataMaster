#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../dsp/common/SmoothedValue.h"
#include "../dsp/common/DecibelUtils.h"
#include <atomic>
#include <memory>
#include <vector>

namespace strata::engine {

enum class ABMode {
    TrackA, // Processed Mastering Signal
    TrackB  // Reference Track
};

/**
 * Intelligent Reference Track Player & Gain Matcher.
 * Features:
 * - High-resolution multi-format reference audio loading (WAV, AIFF, FLAC, OGG)
 * - Automatic sample rate conversion / resampling to host rate
 * - Click-free A/B equal-power crossfading
 * - Automatic Loudness Gain Matching (EBU R128 LUFS offset compensation)
 * - Lock-free, real-time safe buffer swap
 */
class ReferencePlayer final {
public:
    ReferencePlayer();
    ~ReferencePlayer() = default;

    void prepare(double sampleRate, size_t maxBlockSize);
    void reset() noexcept;

    // File loading
    bool loadAudioFile(const juce::File& file);
    void loadDirectBuffer(juce::AudioBuffer<float>&& buffer, double fileSampleRate);
    void unloadReference() noexcept;

    [[nodiscard]] bool hasReferenceLoaded() const noexcept {
        return isLoaded_.load(std::memory_order_relaxed);
    }
    [[nodiscard]] juce::String getLoadedFileName() const {
        return loadedFileName_;
    }
    [[nodiscard]] double getReferenceSampleRate() const noexcept {
        return referenceSampleRate_;
    }
    [[nodiscard]] double getReferenceLengthSeconds() const noexcept;

    // A/B Selection & Gain Match
    void setABMode(ABMode mode) noexcept;
    [[nodiscard]] ABMode getABMode() const noexcept {
        return currentMode_.load(std::memory_order_relaxed);
    }

    void setGainMatchEnabled(bool enabled) noexcept {
        gainMatchEnabled_.store(enabled, std::memory_order_relaxed);
    }
    [[nodiscard]] bool isGainMatchEnabled() const noexcept {
        return gainMatchEnabled_.load(std::memory_order_relaxed);
    }

    void setGainMatchOffsetDb(float offsetDb) noexcept {
        gainMatchOffsetDb_.store(std::clamp(offsetDb, -30.0f, 30.0f), std::memory_order_relaxed);
    }
    [[nodiscard]] float getGainMatchOffsetDb() const noexcept {
        return gainMatchOffsetDb_.load(std::memory_order_relaxed);
    }

    // Loop & Transport
    void setLooping(bool loop) noexcept {
        looping_.store(loop, std::memory_order_relaxed);
    }
    void setPlayheadPosition(int64_t sampleIndex) noexcept {
        playheadPosition_.store(sampleIndex, std::memory_order_relaxed);
    }
    [[nodiscard]] int64_t getPlayheadPosition() const noexcept {
        return playheadPosition_.load(std::memory_order_relaxed);
    }

    /**
     * Real-time audio callback process.
     * In-place crossfades between master track (in `left` and `right`) and reference track.
     */
    void process(float* left, float* right, size_t numSamples) noexcept;

    /** Helper to calculate EBU R128 gain match offset */
    static float computeGainMatchOffset(float masterIntegratedLufs, float referenceIntegratedLufs) noexcept {
        if (masterIntegratedLufs < -60.0f || referenceIntegratedLufs < -60.0f) {
            return 0.0f;
        }
        return masterIntegratedLufs - referenceIntegratedLufs;
    }

private:
    void resampleAndStore(const juce::AudioBuffer<float>& sourceBuffer, double sourceRate);

    double hostSampleRate_{ 44100.0 };
    double referenceSampleRate_{ 44100.0 };

    juce::AudioFormatManager formatManager_;
    juce::String loadedFileName_;

    // Thread-safe reference audio storage
    juce::AudioBuffer<float> referenceBuffer_;
    std::atomic<bool> isLoaded_{ false };
    std::atomic<int64_t> totalReferenceSamples_{ 0 };
    std::atomic<int64_t> playheadPosition_{ 0 };
    std::atomic<bool> looping_{ true };

    // A/B crossfade
    std::atomic<ABMode> currentMode_{ ABMode::TrackA };
    dsp::SmoothedValue<float> crossfadeSmoother_;

    // Gain match
    std::atomic<bool> gainMatchEnabled_{ false };
    std::atomic<float> gainMatchOffsetDb_{ 0.0f };
    dsp::SmoothedValue<float> gainMatchSmoother_;
};

} // namespace strata::engine
