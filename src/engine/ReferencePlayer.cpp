#include "ReferencePlayer.h"
#include <juce_dsp/juce_dsp.h>
#include <algorithm>

namespace strata::engine {

ReferencePlayer::ReferencePlayer() {
    formatManager_.registerBasicFormats();
}

void ReferencePlayer::prepare(double sampleRate, size_t /*maxBlockSize*/) {
    hostSampleRate_ = sampleRate;

    crossfadeSmoother_.reset(sampleRate, 0.020f); // 20 ms click-free crossfade
    crossfadeSmoother_.setCurrentAndTargetValue(0.0f);

    gainMatchSmoother_.reset(sampleRate, 0.020f);
    gainMatchSmoother_.setCurrentAndTargetValue(1.0f);
}

void ReferencePlayer::reset() noexcept {
    crossfadeSmoother_.setCurrentAndTargetValue(0.0f);
    gainMatchSmoother_.setCurrentAndTargetValue(1.0f);
    playheadPosition_.store(0, std::memory_order_relaxed);
}

double ReferencePlayer::getReferenceLengthSeconds() const noexcept {
    const int64_t total = totalReferenceSamples_.load(std::memory_order_relaxed);
    return (hostSampleRate_ > 0.0) ? (static_cast<double>(total) / hostSampleRate_) : 0.0;
}

void ReferencePlayer::setABMode(ABMode mode) noexcept {
    currentMode_.store(mode, std::memory_order_relaxed);
}

void ReferencePlayer::unloadReference() noexcept {
    isLoaded_.store(false, std::memory_order_relaxed);
    totalReferenceSamples_.store(0, std::memory_order_relaxed);
    playheadPosition_.store(0, std::memory_order_relaxed);
    loadedFileName_.clear();
}

bool ReferencePlayer::loadAudioFile(const juce::File& file) {
    if (!file.existsAsFile()) {
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager_.createReaderFor(file));
    if (!reader) {
        return false;
    }

    const int numChannels = std::min(2, static_cast<int>(reader->numChannels));
    const int64_t numSamples = static_cast<int64_t>(reader->lengthInSamples);
    if (numSamples <= 0) {
        return false;
    }

    juce::AudioBuffer<float> tempBuffer(2, static_cast<int>(numSamples));
    if (numChannels == 1) {
        reader->read(&tempBuffer, 0, static_cast<int>(numSamples), 0, true, false);
        // Duplicate mono to stereo
        tempBuffer.copyFrom(1, 0, tempBuffer, 0, 0, static_cast<int>(numSamples));
    } else {
        reader->read(&tempBuffer, 0, static_cast<int>(numSamples), 0, true, true);
    }

    referenceSampleRate_ = reader->sampleRate;
    loadedFileName_ = file.getFileName();

    resampleAndStore(tempBuffer, reader->sampleRate);
    return true;
}

void ReferencePlayer::loadDirectBuffer(juce::AudioBuffer<float>&& buffer, double fileSampleRate) {
    referenceSampleRate_ = fileSampleRate;
    loadedFileName_ = "Direct Memory Buffer";
    resampleAndStore(buffer, fileSampleRate);
}

void ReferencePlayer::resampleAndStore(const juce::AudioBuffer<float>& sourceBuffer, double sourceRate) {
    if (std::abs(sourceRate - hostSampleRate_) < 1.0 || hostSampleRate_ <= 0.0) {
        // Direct copy if sample rates match
        referenceBuffer_.makeCopyOf(sourceBuffer);
    } else {
        // Resample using linear or windowed sinc interpolation to host rate
        const double ratio = sourceRate / hostSampleRate_;
        const int outLength = static_cast<int>(static_cast<double>(sourceBuffer.getNumSamples()) / ratio);

        juce::AudioBuffer<float> resampled(2, outLength);
        const float* srcL = sourceBuffer.getReadPointer(0);
        const float* srcR = (sourceBuffer.getNumChannels() > 1) ? sourceBuffer.getReadPointer(1) : srcL;
        float* dstL = resampled.getWritePointer(0);
        float* dstR = resampled.getWritePointer(1);

        for (int i = 0; i < outLength; ++i) {
            const double srcPos = static_cast<double>(i) * ratio;
            const int idx0 = static_cast<int>(srcPos);
            const int idx1 = std::min(idx0 + 1, sourceBuffer.getNumSamples() - 1);
            const float frac = static_cast<float>(srcPos - static_cast<double>(idx0));

            dstL[i] = srcL[idx0] + frac * (srcL[idx1] - srcL[idx0]);
            dstR[i] = srcR[idx0] + frac * (srcR[idx1] - srcR[idx0]);
        }

        referenceBuffer_ = std::move(resampled);
    }

    totalReferenceSamples_.store(referenceBuffer_.getNumSamples(), std::memory_order_relaxed);
    playheadPosition_.store(0, std::memory_order_relaxed);
    isLoaded_.store(true, std::memory_order_relaxed);
}

void ReferencePlayer::process(float* left, float* right, size_t numSamples) noexcept {
    const bool loaded = isLoaded_.load(std::memory_order_relaxed);
    const ABMode mode = currentMode_.load(std::memory_order_relaxed);
    const float targetFade = (mode == ABMode::TrackB && loaded) ? 1.0f : 0.0f;
    crossfadeSmoother_.setTargetValue(targetFade);

    const bool gainMatch = gainMatchEnabled_.load(std::memory_order_relaxed);
    const float offsetDb = gainMatchOffsetDb_.load(std::memory_order_relaxed);
    const float targetGain = (gainMatch && loaded) ? dsp::decibelsToGain(offsetDb) : 1.0f;
    gainMatchSmoother_.setTargetValue(targetGain);

    if (!loaded && crossfadeSmoother_.getCurrentValue() < 0.0001f && targetFade < 0.0001f) {
        return;
    }

    const int totalSamples = static_cast<int>(totalReferenceSamples_.load(std::memory_order_relaxed));
    if (totalSamples <= 0) {
        return;
    }

    const float* refL = referenceBuffer_.getReadPointer(0);
    const float* refR = referenceBuffer_.getReadPointer(1);
    int64_t pos = playheadPosition_.load(std::memory_order_relaxed);
    const bool loop = looping_.load(std::memory_order_relaxed);

    for (size_t i = 0; i < numSamples; ++i) {
        const float fade = crossfadeSmoother_.getNextValue();
        const float gain = gainMatchSmoother_.getNextValue();

        float rL = 0.0f;
        float rR = 0.0f;

        if (pos >= 0 && pos < totalSamples) {
            rL = refL[pos] * gain;
            rR = refR[pos] * gain;
            ++pos;
        } else if (loop && totalSamples > 0) {
            pos = 0;
            rL = refL[pos] * gain;
            rR = refR[pos] * gain;
            ++pos;
        }

        if (fade > 0.0001f) {
            left[i]  = (1.0f - fade) * left[i]  + fade * rL;
            right[i] = (1.0f - fade) * right[i] + fade * rR;
        }
    }

    playheadPosition_.store(pos, std::memory_order_relaxed);
}

} // namespace strata::engine
