#include "StrataAudioProcessor.h"
#include "ParameterLayout.h"
#include "ParameterIDs.h"
#include "../dsp/common/DenormalGuard.h"
#include "../dsp/common/DecibelUtils.h"
#include "../gui/StrataAudioEditor.h"
#include <cmath>
#include <cstdlib>

namespace strata::engine {

StrataAudioProcessor::StrataAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", ParameterLayout::createParameterLayout()),
      presetManager_(apvts_) {

    formatManager_.registerBasicFormats();

    inputGainParam_       = apvts_.getRawParameterValue(param::InputGain.data());
    outputGainParam_      = apvts_.getRawParameterValue(param::OutputGain.data());
    bypassParam_          = apvts_.getRawParameterValue(param::Bypass.data());
    eqBypassParam_        = apvts_.getRawParameterValue(param::EQBypass.data());
    dynamicsBypassParam_  = apvts_.getRawParameterValue(param::DynamicsBypass.data());
    stereoModeParam_      = apvts_.getRawParameterValue(param::StereoMode.data());
    stereoWidthParam_     = apvts_.getRawParameterValue(param::StereoWidth.data());

    crossoverSplit1Param_ = apvts_.getRawParameterValue(param::CrossoverSplit1.data());
    crossoverSplit2Param_ = apvts_.getRawParameterValue(param::CrossoverSplit2.data());
    crossoverSplit3Param_ = apvts_.getRawParameterValue(param::CrossoverSplit3.data());

    exciterBypassParam_       = apvts_.getRawParameterValue(param::ExciterBypass.data());
    exciterOversamplingParam_ = apvts_.getRawParameterValue(param::ExciterOversampling.data());
    transientBypassParam_     = apvts_.getRawParameterValue(param::TransientBypass.data());

    for (size_t b = 0; b < dsp::NUM_EQ_BANDS; ++b) {
        eqBandParams_[b].enable     = apvts_.getRawParameterValue(param::getEQBandEnableID(b));
        eqBandParams_[b].type       = apvts_.getRawParameterValue(param::getEQBandTypeID(b));
        eqBandParams_[b].freq       = apvts_.getRawParameterValue(param::getEQBandFreqID(b));
        eqBandParams_[b].gain       = apvts_.getRawParameterValue(param::getEQBandGainID(b));
        eqBandParams_[b].q          = apvts_.getRawParameterValue(param::getEQBandQID(b));
        eqBandParams_[b].dynEnable  = apvts_.getRawParameterValue(param::getEQBandDynEnableID(b));
        eqBandParams_[b].dynThresh  = apvts_.getRawParameterValue(param::getEQBandDynThreshID(b));
        eqBandParams_[b].dynRange   = apvts_.getRawParameterValue(param::getEQBandDynRangeID(b));
    }

    for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
        dynBandParams_[b].enable     = apvts_.getRawParameterValue(param::getDynBandEnableID(b));
        dynBandParams_[b].bypass     = apvts_.getRawParameterValue(param::getDynBandBypassID(b));
        dynBandParams_[b].solo       = apvts_.getRawParameterValue(param::getDynBandSoloID(b));
        dynBandParams_[b].mute       = apvts_.getRawParameterValue(param::getDynBandMuteID(b));
        dynBandParams_[b].thresh     = apvts_.getRawParameterValue(param::getDynBandThreshID(b));
        dynBandParams_[b].ratio      = apvts_.getRawParameterValue(param::getDynBandRatioID(b));
        dynBandParams_[b].attack     = apvts_.getRawParameterValue(param::getDynBandAttackID(b));
        dynBandParams_[b].release    = apvts_.getRawParameterValue(param::getDynBandReleaseID(b));
        dynBandParams_[b].makeup     = apvts_.getRawParameterValue(param::getDynBandMakeupID(b));
        dynBandParams_[b].autoMakeup = apvts_.getRawParameterValue(param::getDynBandAutoMakeupID(b));

        exciterBandParams_[b].enable = apvts_.getRawParameterValue(param::getExciterBandEnableID(b));
        exciterBandParams_[b].mode   = apvts_.getRawParameterValue(param::getExciterBandModeID(b));
        exciterBandParams_[b].drive  = apvts_.getRawParameterValue(param::getExciterBandDriveID(b));
        exciterBandParams_[b].mix    = apvts_.getRawParameterValue(param::getExciterBandMixID(b));

        transientBandParams_[b].enable  = apvts_.getRawParameterValue(param::getTransientBandEnableID(b));
        transientBandParams_[b].attack  = apvts_.getRawParameterValue(param::getTransientBandAttackID(b));
        transientBandParams_[b].sustain = apvts_.getRawParameterValue(param::getTransientBandSustainID(b));
    }

    stereoMonoMakerEnableParam_  = apvts_.getRawParameterValue(param::StereoMonoMakerEnable.data());
    stereoMonoMakerFreqParam_    = apvts_.getRawParameterValue(param::StereoMonoMakerFreq.data());
    stereoStereoizerEnableParam_ = apvts_.getRawParameterValue(param::StereoStereoizerEnable.data());
    stereoStereoizerAmountParam_ = apvts_.getRawParameterValue(param::StereoStereoizerAmount.data());

    clipperEnableParam_  = apvts_.getRawParameterValue(param::ClipperEnable.data());
    clipperCeilingParam_ = apvts_.getRawParameterValue(param::ClipperCeiling.data());
    clipperKneeParam_    = apvts_.getRawParameterValue(param::ClipperKnee.data());

    limiterBypassParam_    = apvts_.getRawParameterValue(param::LimiterBypass.data());
    limiterCeilingParam_   = apvts_.getRawParameterValue(param::LimiterCeiling.data());
    limiterThresholdParam_ = apvts_.getRawParameterValue(param::LimiterThreshold.data());
    limiterReleaseParam_   = apvts_.getRawParameterValue(param::LimiterRelease.data());
    limiterLookaheadParam_ = apvts_.getRawParameterValue(param::LimiterLookahead.data());
}

bool StrataAudioProcessor::loadAuditionAudio(const juce::File& file) {
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
        tempBuffer.copyFrom(1, 0, tempBuffer, 0, 0, static_cast<int>(numSamples));
    } else {
        reader->read(&tempBuffer, 0, static_cast<int>(numSamples), 0, true, true);
    }

    const double hostRate = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
    if (std::abs(reader->sampleRate - hostRate) < 1.0) {
        auditionBuffer_ = std::move(tempBuffer);
    } else {
        const double ratio = reader->sampleRate / hostRate;
        const int outLength = static_cast<int>(static_cast<double>(numSamples) / ratio);
        juce::AudioBuffer<float> resampled(2, outLength);
        const float* srcL = tempBuffer.getReadPointer(0);
        const float* srcR = tempBuffer.getReadPointer(1);
        float* dstL = resampled.getWritePointer(0);
        float* dstR = resampled.getWritePointer(1);

        for (int i = 0; i < outLength; ++i) {
            const double srcPos = static_cast<double>(i) * ratio;
            const int idx0 = static_cast<int>(srcPos);
            const int idx1 = std::min(idx0 + 1, static_cast<int>(numSamples) - 1);
            const float frac = static_cast<float>(srcPos - static_cast<double>(idx0));
            dstL[i] = srcL[idx0] + frac * (srcL[idx1] - srcL[idx0]);
            dstR[i] = srcR[idx0] + frac * (srcR[idx1] - srcR[idx0]);
        }
        auditionBuffer_ = std::move(resampled);
    }

    auditionFileName_ = file.getFileName();
    auditionPlayhead_.store(0, std::memory_order_relaxed);
    auditionPlaying_.store(true, std::memory_order_relaxed);
    return true;
}

void StrataAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    const float inGain = inputGainParam_ ? inputGainParam_->load(std::memory_order_relaxed) : 0.0f;
    const float outGain = outputGainParam_ ? outputGainParam_->load(std::memory_order_relaxed) : 0.0f;
    const float width = stereoWidthParam_ ? stereoWidthParam_->load(std::memory_order_relaxed) : 1.0f;

    inputGainSmoother_.reset(sampleRate, 0.02);
    inputGainSmoother_.setCurrentAndTargetValue(dsp::decibelsToGain(inGain));
    outputGainSmoother_.reset(sampleRate, 0.02);
    outputGainSmoother_.setCurrentAndTargetValue(dsp::decibelsToGain(outGain));
    stereoWidthSmoother_.reset(sampleRate, 0.02);
    stereoWidthSmoother_.setCurrentAndTargetValue(width);

    stereoProcessor_.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    dynamicEqualizer_.prepare(sampleRate);
    multibandDynamics_.prepare(sampleRate);
    exciterCrossover_.prepare(sampleRate);
    transientCrossover_.prepare(sampleRate);

    const size_t bufSize = std::max(static_cast<size_t>(samplesPerBlock), size_t{2048});
    for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
        exciterBandBuffersL_[b].assign(bufSize, 0.0f);
        exciterBandBuffersR_[b].assign(bufSize, 0.0f);
        transientBandBuffersL_[b].assign(bufSize, 0.0f);
        transientBandBuffersR_[b].assign(bufSize, 0.0f);
    }

    dsp::OversamplingFactor factor = dsp::OversamplingFactor::X4;
    lastOversamplingIdx_ = exciterOversamplingParam_ ? static_cast<int>(exciterOversamplingParam_->load(std::memory_order_relaxed)) : 2;
    switch (lastOversamplingIdx_) {
        case 0: factor = dsp::OversamplingFactor::None; break;
        case 1: factor = dsp::OversamplingFactor::X2;   break;
        case 2: factor = dsp::OversamplingFactor::X4;   break;
        case 3: factor = dsp::OversamplingFactor::X8;   break;
        default: factor = dsp::OversamplingFactor::X4;  break;
    }
    saturator_.prepare(sampleRate, static_cast<size_t>(samplesPerBlock), factor);
    maseratiProcessor_.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));

    transientProcessor_.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));

    truePeakLimiter_.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    lastLookaheadMs_ = limiterLookaheadParam_ ? limiterLookaheadParam_->load(std::memory_order_relaxed) : 3.0f;
    truePeakLimiter_.setLookaheadMs(lastLookaheadMs_);

    correlationMeter_.prepare(sampleRate);
    bs1770Meter_.prepare(sampleRate);
    truePeakMeter_.prepare(sampleRate);
    spectrumAnalyzer_.prepare(sampleRate);
    featureExtractor_.prepare(sampleRate);
    referencePlayer_.prepare(sampleRate, static_cast<size_t>(samplesPerBlock));
    autoMasterEngine_.prepare(sampleRate);

    const int totalLatency = static_cast<int>(saturator_.getLatencyInSamples() + truePeakLimiter_.getLatencyInSamples());
    setLatencySamples(totalLatency);
}

void StrataAudioProcessor::releaseResources() {
    lastOversamplingIdx_ = -1;
    lastLookaheadMs_ = -1.0f;
    stereoProcessor_.reset();
    dynamicEqualizer_.reset();
    multibandDynamics_.reset();
    exciterCrossover_.reset();
    transientCrossover_.reset();
    saturator_.reset();
    maseratiProcessor_.reset();
    transientProcessor_.reset();
    truePeakLimiter_.reset();
    correlationMeter_.reset();
    bs1770Meter_.reset();
    truePeakMeter_.reset();
    spectrumAnalyzer_.reset();
    featureExtractor_.reset();
    referencePlayer_.reset();
    autoMasterEngine_.reset();
}

bool StrataAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();

    if (mainOutput != juce::AudioChannelSet::stereo()) {
        return false;
    }
    return (mainInput == juce::AudioChannelSet::stereo() ||
            mainInput == juce::AudioChannelSet::mono() ||
            mainInput == juce::AudioChannelSet::disabled());
}

void StrataAudioProcessor::updateEQParametersFromState() noexcept {
    for (size_t b = 0; b < dsp::NUM_EQ_BANDS; ++b) {
        auto& p = eqBandParams_[b];
        if (!p.enable) {
            continue;
        }

        dsp::DynamicBandSettings s;
        s.enabled        = (p.enable->load(std::memory_order_relaxed) > 0.5f);
        s.type           = static_cast<dsp::FilterType>(static_cast<int>(p.type->load(std::memory_order_relaxed)));
        s.frequencyHz    = p.freq->load(std::memory_order_relaxed);
        s.gainDb         = p.gain->load(std::memory_order_relaxed);
        s.q              = p.q->load(std::memory_order_relaxed);
        s.dynamicEnabled = (p.dynEnable->load(std::memory_order_relaxed) > 0.5f);
        s.thresholdDb    = p.dynThresh->load(std::memory_order_relaxed);
        s.dynamicRangeDb = p.dynRange->load(std::memory_order_relaxed);
        s.attackMs       = 10.0f;
        s.releaseMs      = 100.0f;

        dynamicEqualizer_.setBandSettings(b, s);
    }
}

void StrataAudioProcessor::updateDynamicsParametersFromState() noexcept {
    if (crossoverSplit1Param_ && crossoverSplit2Param_ && crossoverSplit3Param_) {
        const float f1 = crossoverSplit1Param_->load(std::memory_order_relaxed);
        const float f2 = crossoverSplit2Param_->load(std::memory_order_relaxed);
        const float f3 = crossoverSplit3Param_->load(std::memory_order_relaxed);
        multibandDynamics_.setCrossoverFrequencies(f1, f2, f3);
        exciterCrossover_.setCrossoverFrequencies(f1, f2, f3);
        transientCrossover_.setCrossoverFrequencies(f1, f2, f3);
    }

    for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
        auto& p = dynBandParams_[b];
        if (!p.enable) {
            continue;
        }

        dsp::BandDynamicsSettings s;
        s.enabled      = (p.enable->load(std::memory_order_relaxed) > 0.5f);
        s.bypass       = (p.bypass->load(std::memory_order_relaxed) > 0.5f);
        s.solo         = (p.solo->load(std::memory_order_relaxed) > 0.5f);
        s.mute         = (p.mute->load(std::memory_order_relaxed) > 0.5f);
        s.thresholdDb  = p.thresh->load(std::memory_order_relaxed);
        s.ratio        = p.ratio->load(std::memory_order_relaxed);
        s.attackMs     = p.attack->load(std::memory_order_relaxed);
        s.releaseMs    = p.release->load(std::memory_order_relaxed);
        s.makeupGainDb = p.makeup->load(std::memory_order_relaxed);
        s.autoMakeup   = (p.autoMakeup->load(std::memory_order_relaxed) > 0.5f);

        multibandDynamics_.setBandSettings(b, s);
    }
}

void StrataAudioProcessor::updateExciterParametersFromState() noexcept {
    if (exciterOversamplingParam_) {
        const int osIdx = static_cast<int>(exciterOversamplingParam_->load(std::memory_order_relaxed));
        if (osIdx != lastOversamplingIdx_) {
            lastOversamplingIdx_ = osIdx;
            dsp::OversamplingFactor factor = dsp::OversamplingFactor::X4;
            switch (osIdx) {
                case 0: factor = dsp::OversamplingFactor::None; break;
                case 1: factor = dsp::OversamplingFactor::X2;   break;
                case 2: factor = dsp::OversamplingFactor::X4;   break;
                case 3: factor = dsp::OversamplingFactor::X8;   break;
                default: factor = dsp::OversamplingFactor::X4;  break;
            }
            saturator_.setOversamplingFactor(factor);
            const int totalLatency = static_cast<int>(saturator_.getLatencyInSamples() + truePeakLimiter_.getLatencyInSamples());
            if (getLatencySamples() != totalLatency) {
                setLatencySamples(totalLatency);
            }
        }
    }

    for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
        auto& p = exciterBandParams_[b];
        if (!p.enable) {
            continue;
        }

        dsp::BandExciterSettings s;
        s.enabled = (p.enable->load(std::memory_order_relaxed) > 0.5f);
        s.mode    = static_cast<dsp::SaturationMode>(static_cast<int>(p.mode->load(std::memory_order_relaxed)));
        s.drive   = p.drive->load(std::memory_order_relaxed) / 100.0f;
        s.mix     = p.mix->load(std::memory_order_relaxed) / 100.0f;

        saturator_.setBandSettings(b, s);
    }
}

void StrataAudioProcessor::updateTransientParametersFromState() noexcept {
    for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
        auto& p = transientBandParams_[b];
        if (!p.enable) {
            continue;
        }

        dsp::BandTransientSettings s;
        s.enabled       = (p.enable->load(std::memory_order_relaxed) > 0.5f);
        s.attackGainDb  = p.attack->load(std::memory_order_relaxed);
        s.sustainGainDb = p.sustain->load(std::memory_order_relaxed);

        transientProcessor_.setBandSettings(b, s);
    }
}

void StrataAudioProcessor::updateStereoParametersFromState() noexcept {
    if (stereoMonoMakerEnableParam_) {
        stereoProcessor_.setMonoMakerEnabled(stereoMonoMakerEnableParam_->load(std::memory_order_relaxed) > 0.5f);
    }
    if (stereoMonoMakerFreqParam_) {
        stereoProcessor_.setMonoMakerFrequency(stereoMonoMakerFreqParam_->load(std::memory_order_relaxed));
    }
    if (stereoStereoizerEnableParam_) {
        stereoProcessor_.setStereoizerEnabled(stereoStereoizerEnableParam_->load(std::memory_order_relaxed) > 0.5f);
    }
    if (stereoStereoizerAmountParam_) {
        stereoProcessor_.setStereoizerAmount(stereoStereoizerAmountParam_->load(std::memory_order_relaxed) / 100.0f);
    }
}

void StrataAudioProcessor::updateLimiterParametersFromState() noexcept {
    if (clipperEnableParam_) {
        truePeakLimiter_.getClipper().setEnabled(clipperEnableParam_->load(std::memory_order_relaxed) > 0.5f);
    }
    if (clipperCeilingParam_) {
        truePeakLimiter_.getClipper().setCeilingDb(clipperCeilingParam_->load(std::memory_order_relaxed));
    }
    if (clipperKneeParam_) {
        truePeakLimiter_.getClipper().setKneeDb(clipperKneeParam_->load(std::memory_order_relaxed));
    }

    if (limiterCeilingParam_) {
        truePeakLimiter_.setCeilingDb(limiterCeilingParam_->load(std::memory_order_relaxed));
    }
    if (limiterThresholdParam_) {
        truePeakLimiter_.setThresholdDb(limiterThresholdParam_->load(std::memory_order_relaxed));
    }
    if (limiterReleaseParam_) {
        truePeakLimiter_.setReleaseMs(limiterReleaseParam_->load(std::memory_order_relaxed));
    }
    if (limiterLookaheadParam_) {
        const float lookaheadMs = limiterLookaheadParam_->load(std::memory_order_relaxed);
        if (std::abs(lookaheadMs - lastLookaheadMs_) > 1e-4f) {
            lastLookaheadMs_ = lookaheadMs;
            truePeakLimiter_.setLookaheadMs(lookaheadMs);
            const int totalLatency = static_cast<int>(saturator_.getLatencyInSamples() + truePeakLimiter_.getLatencyInSamples());
            if (getLatencySamples() != totalLatency) {
                setLatencySamples(totalLatency);
            }
        }
    }
}

void StrataAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/) {
    dsp::DenormalGuard denormalGuard;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    if (numChannels < 2 || numSamples == 0) {
        return;
    }

    // Expand mono input to stereo if host provided mono input
    if (getTotalNumInputChannels() == 1) {
        buffer.copyFrom(1, 0, buffer.getReadPointer(0), numSamples);
    }

    auto* leftChannel  = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    // Audition track playback or test tone injection if active
    const bool isAuditionOn = auditionPlaying_.load(std::memory_order_relaxed);
    const int testToneMode = auditionTestTone_.load(std::memory_order_relaxed);

    if (isAuditionOn && auditionBuffer_.getNumSamples() > 0) {
        const int totalAudition = auditionBuffer_.getNumSamples();
        const float* srcL = auditionBuffer_.getReadPointer(0);
        const float* srcR = (auditionBuffer_.getNumChannels() > 1) ? auditionBuffer_.getReadPointer(1) : srcL;
        int64_t pos = auditionPlayhead_.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i) {
            if (pos >= totalAudition) {
                if (auditionLoop_.load(std::memory_order_relaxed)) {
                    pos = 0;
                } else {
                    break;
                }
            }
            leftChannel[i]  += srcL[pos];
            rightChannel[i] += srcR[pos];
            ++pos;
        }
        auditionPlayhead_.store(pos, std::memory_order_relaxed);
    } else if (testToneMode == 1) {
        // Pink Noise (-18 dBFS calibration)
        static float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            const float white = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f) * 0.125f;
            b0 = 0.99886f * b0 + white * 0.0555179f;
            b1 = 0.99332f * b1 + white * 0.0750759f;
            b2 = 0.96900f * b2 + white * 0.1538520f;
            b3 = 0.86650f * b3 + white * 0.3104856f;
            b4 = 0.55000f * b4 + white * 0.5329522f;
            b5 = -0.7616f * b5 - white * 0.0168980f;
            const float pink = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f) * 0.12f;
            b6 = white * 0.115926f;
            leftChannel[i]  += pink;
            rightChannel[i] += pink;
        }
    } else if (testToneMode == 2) {
        // 1 kHz Sine (-18 dBFS calibration)
        static double phase = 0.0;
        const double phaseInc = 6.283185307179586 * 1000.0 / (getSampleRate() > 0.0 ? getSampleRate() : 44100.0);
        constexpr float amp = 0.125f; // ~ -18 dBFS
        for (int i = 0; i < numSamples; ++i) {
            const float s = amp * static_cast<float>(std::sin(phase));
            phase += phaseInc;
            if (phase >= 6.283185307179586) phase -= 6.283185307179586;
            leftChannel[i]  += s;
            rightChannel[i] += s;
        }
    }

    // Measure raw input peak
    float inPeakL = buffer.getMagnitude(0, 0, numSamples);
    float inPeakR = buffer.getMagnitude(1, 0, numSamples);
    inputPeakL_.store(inPeakL, std::memory_order_relaxed);
    inputPeakR_.store(inPeakR, std::memory_order_relaxed);

    const bool isBypassed = (bypassParam_->load(std::memory_order_relaxed) > 0.5f);
    if (isBypassed) {
        outputPeakL_.store(inPeakL, std::memory_order_relaxed);
        outputPeakR_.store(inPeakR, std::memory_order_relaxed);
        correlationMeter_.process(leftChannel, rightChannel, static_cast<size_t>(numSamples));
        bs1770Meter_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples));
        truePeakMeter_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples));
        spectrumAnalyzer_.pushAudioBlock(leftChannel, rightChannel, static_cast<size_t>(numSamples));
        return;
    }

    // 1. Target parameter updates
    const float inGainDb    = inputGainParam_->load(std::memory_order_relaxed);
    const float outGainDb   = outputGainParam_->load(std::memory_order_relaxed);
    const float targetWidth = stereoWidthParam_->load(std::memory_order_relaxed);
    const int   modeIdx     = static_cast<int>(stereoModeParam_->load(std::memory_order_relaxed));

    inputGainSmoother_.setTargetValue(dsp::decibelsToGain(inGainDb));
    outputGainSmoother_.setTargetValue(dsp::decibelsToGain(outGainDb));
    stereoWidthSmoother_.setTargetValue(targetWidth);

    // 2. Input Gain Stage
    for (int i = 0; i < numSamples; ++i) {
        const float gain = inputGainSmoother_.getNextValue();
        leftChannel[i]  *= gain;
        rightChannel[i] *= gain;
    }

    // 3. Dynamic Equalizer Stage
    const bool isEQBypassed = (eqBypassParam_->load(std::memory_order_relaxed) > 0.5f);
    if (!isEQBypassed) {
        updateEQParametersFromState();
        dynamicEqualizer_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    }

    // 4. Multiband Dynamics Stage
    const bool isDynBypassed = (dynamicsBypassParam_->load(std::memory_order_relaxed) > 0.5f);
    if (!isDynBypassed) {
        updateDynamicsParametersFromState();
        multibandDynamics_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    }

    // 5. Multiband Transient & Sustain Processing Stage
    const bool isTransientBypassed = (transientBypassParam_ && transientBypassParam_->load(std::memory_order_relaxed) > 0.5f);
    if (!isTransientBypassed) {
        updateTransientParametersFromState();
        bool anyTransientActive = false;
        for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
            const auto& s = transientProcessor_.getBandSettings(b);
            if (s.enabled && (std::abs(s.attackGainDb) > 0.05f || std::abs(s.sustainGainDb) > 0.05f)) {
                anyTransientActive = true;
                break;
            }
        }
        if (anyTransientActive) {
            for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
                if (transientBandBuffersL_[b].size() < static_cast<size_t>(numSamples)) {
                    transientBandBuffersL_[b].resize(static_cast<size_t>(numSamples));
                    transientBandBuffersR_[b].resize(static_cast<size_t>(numSamples));
                }
            }
            std::array<float*, dsp::NUM_CROSSOVER_BANDS> tPtrsL = {
                transientBandBuffersL_[0].data(), transientBandBuffersL_[1].data(),
                transientBandBuffersL_[2].data(), transientBandBuffersL_[3].data()
            };
            std::array<float*, dsp::NUM_CROSSOVER_BANDS> tPtrsR = {
                transientBandBuffersR_[0].data(), transientBandBuffersR_[1].data(),
                transientBandBuffersR_[2].data(), transientBandBuffersR_[3].data()
            };

            transientCrossover_.splitChannel(0, leftChannel, tPtrsL.data(), static_cast<size_t>(numSamples));
            transientCrossover_.splitChannel(1, rightChannel, tPtrsR.data(), static_cast<size_t>(numSamples));

            float* tMultiband[dsp::NUM_CROSSOVER_BANDS][2] = {
                { tPtrsL[0], tPtrsR[0] },
                { tPtrsL[1], tPtrsR[1] },
                { tPtrsL[2], tPtrsR[2] },
                { tPtrsL[3], tPtrsR[3] }
            };
            transientProcessor_.processMultiband(tMultiband, static_cast<size_t>(numSamples));

            dsp::CrossoverFilterbank::sumChannel(tPtrsL.data(), leftChannel, static_cast<size_t>(numSamples));
            dsp::CrossoverFilterbank::sumChannel(tPtrsR.data(), rightChannel, static_cast<size_t>(numSamples));
        }
    }

    // 6. Multiband Harmonic Exciter & Saturation Stage
    const bool isExciterBypassed = (exciterBypassParam_ && exciterBypassParam_->load(std::memory_order_relaxed) > 0.5f);
    if (!isExciterBypassed) {
        updateExciterParametersFromState();
        bool anyExciterActive = false;
        for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
            const auto& s = saturator_.getBandSettings(b);
            if (s.enabled && s.drive > 0.001f) {
                anyExciterActive = true;
                break;
            }
        }
        if (anyExciterActive) {
            for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
                if (exciterBandBuffersL_[b].size() < static_cast<size_t>(numSamples)) {
                    exciterBandBuffersL_[b].resize(static_cast<size_t>(numSamples));
                    exciterBandBuffersR_[b].resize(static_cast<size_t>(numSamples));
                }
            }
            std::array<float*, dsp::NUM_CROSSOVER_BANDS> ePtrsL = {
                exciterBandBuffersL_[0].data(), exciterBandBuffersL_[1].data(),
                exciterBandBuffersL_[2].data(), exciterBandBuffersL_[3].data()
            };
            std::array<float*, dsp::NUM_CROSSOVER_BANDS> ePtrsR = {
                exciterBandBuffersR_[0].data(), exciterBandBuffersR_[1].data(),
                exciterBandBuffersR_[2].data(), exciterBandBuffersR_[3].data()
            };

            exciterCrossover_.splitChannel(0, leftChannel, ePtrsL.data(), static_cast<size_t>(numSamples));
            exciterCrossover_.splitChannel(1, rightChannel, ePtrsR.data(), static_cast<size_t>(numSamples));

            float* eMultiband[dsp::NUM_CROSSOVER_BANDS][2] = {
                { ePtrsL[0], ePtrsR[0] },
                { ePtrsL[1], ePtrsR[1] },
                { ePtrsL[2], ePtrsR[2] },
                { ePtrsL[3], ePtrsR[3] }
            };
            saturator_.processMultiband(eMultiband, static_cast<size_t>(numSamples));

            dsp::CrossoverFilterbank::sumChannel(ePtrsL.data(), leftChannel, static_cast<size_t>(numSamples));
            dsp::CrossoverFilterbank::sumChannel(ePtrsR.data(), rightChannel, static_cast<size_t>(numSamples));
        }
    }

    // 6.5 Maserati Analog Tone Processor Stage
    const bool isMaseratiBypassed = (maseratiBypassParam_ && maseratiBypassParam_->load(std::memory_order_relaxed) > 0.5f);
    if (!isMaseratiBypassed) {
        if (maseratiBassParam_) maseratiProcessor_.setBass(maseratiBassParam_->load(std::memory_order_relaxed));
        if (maseratiTrebleParam_) maseratiProcessor_.setTreble(maseratiTrebleParam_->load(std::memory_order_relaxed));
        if (maseratiSensitivityParam_) maseratiProcessor_.setSensitivity(maseratiSensitivityParam_->load(std::memory_order_relaxed));
        maseratiProcessor_.setEnabled(true);
        maseratiProcessor_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    } else {
        maseratiProcessor_.setEnabled(false);
    }

    // 7. Stereo & Mid/Side Processing Stage
    updateStereoParametersFromState();
    const auto mode = static_cast<dsp::StereoMode>(std::clamp(modeIdx, 0, 3));
    stereoWidthSmoother_.skip(numSamples);
    const float currentWidth = stereoWidthSmoother_.getCurrentValue();
    stereoProcessor_.process(leftChannel, rightChannel, static_cast<size_t>(numSamples), mode, currentWidth);

    // 8. Output Gain Stage
    for (int i = 0; i < numSamples; ++i) {
        const float gain = outputGainSmoother_.getNextValue();
        leftChannel[i]  *= gain;
        rightChannel[i] *= gain;
    }

    // 9. Lookahead True-Peak Limiter Stage
    const bool isLimiterBypassed = (limiterBypassParam_ && limiterBypassParam_->load(std::memory_order_relaxed) > 0.5f);
    if (!isLimiterBypassed) {
        updateLimiterParametersFromState();
        truePeakLimiter_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    }

    // 10. Reference Track A/B Crossfade & Gain Match
    referencePlayer_.process(leftChannel, rightChannel, static_cast<size_t>(numSamples));

    // 11. Phase Correlation & Advanced Mastering Analysis
    correlationMeter_.process(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    bs1770Meter_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    truePeakMeter_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    spectrumAnalyzer_.pushAudioBlock(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    featureExtractor_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples));
    autoMasterEngine_.processBlock(leftChannel, rightChannel, static_cast<size_t>(numSamples));

    // 12. Output Peak Metering
    float outPeakL = buffer.getMagnitude(0, 0, numSamples);
    float outPeakR = buffer.getMagnitude(1, 0, numSamples);
    outputPeakL_.store(outPeakL, std::memory_order_relaxed);
    outputPeakR_.store(outPeakR, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* StrataAudioProcessor::createEditor() {
    return new gui::StrataAudioEditor(*this);
}

void StrataAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void StrataAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts_.state.getType())) {
        apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}


void StrataAudioProcessor::getLissajousPoints(std::array<std::pair<float, float>, LISSAJOUS_SAMPLES>& dest) const noexcept {
    for (size_t i = 0; i < LISSAJOUS_SAMPLES; ++i) {
        dest[i].first = lissajousL_[i].load(std::memory_order_relaxed);
        dest[i].second = lissajousR_[i].load(std::memory_order_relaxed);
    }
}

} // namespace strata::engine

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new strata::engine::StrataAudioProcessor();
}
