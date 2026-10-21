#include "StrataAudioProcessor.h"
#include "ParameterLayout.h"
#include "ParameterIDs.h"
#include "../dsp/common/DenormalGuard.h"
#include "../dsp/common/DecibelUtils.h"
#include "../gui/StrataAudioEditor.h"

namespace strata::engine {

StrataAudioProcessor::StrataAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", ParameterLayout::createParameterLayout()),
      presetManager_(apvts_) {

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
    saturator_.reset();
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
    return (mainInput == juce::AudioChannelSet::stereo());
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

    auto* leftChannel  = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

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
        return;
    }

    // 1. Target parameter updates
    const float inGainDb   = inputGainParam_->load(std::memory_order_relaxed);
    const float outGainDb  = outputGainParam_->load(std::memory_order_relaxed);
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

    // 5. Transient & Sustain Processing Stage
    const bool isTransientBypassed = (transientBypassParam_ && transientBypassParam_->load(std::memory_order_relaxed) > 0.5f);
    if (!isTransientBypassed) {
        updateTransientParametersFromState();
        float avgAttack = 0.0f;
        float avgSustain = 0.0f;
        for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
            const auto& s = transientProcessor_.getBandSettings(b);
            if (s.enabled) {
                avgAttack += s.attackGainDb;
                avgSustain += s.sustainGainDb;
            }
        }
        avgAttack /= static_cast<float>(dsp::NUM_CROSSOVER_BANDS);
        avgSustain /= static_cast<float>(dsp::NUM_CROSSOVER_BANDS);

        transientProcessor_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples), avgAttack, avgSustain);
    }

    // 6. Harmonic Exciter & Saturation Stage
    const bool isExciterBypassed = (exciterBypassParam_ && exciterBypassParam_->load(std::memory_order_relaxed) > 0.5f);
    if (!isExciterBypassed) {
        updateExciterParametersFromState();
        float maxDrive = 0.0f;
        float avgMix = 1.0f;
        dsp::SaturationMode primaryMode = dsp::SaturationMode::Tape;
        for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
            const auto& s = saturator_.getBandSettings(b);
            if (s.enabled && s.drive > maxDrive) {
                maxDrive = s.drive;
                avgMix = s.mix;
                primaryMode = s.mode;
            }
        }
        if (maxDrive > 0.001f) {
            saturator_.processStereo(leftChannel, rightChannel, static_cast<size_t>(numSamples), primaryMode, maxDrive, avgMix);
        }
    }

    // 7. Stereo & Mid/Side Processing
    updateStereoParametersFromState();
    const auto mode = static_cast<dsp::StereoMode>(std::clamp(modeIdx, 0, 3));
    const float currentWidth = stereoWidthSmoother_.getNextValue();
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

} // namespace strata::engine

// JUCE Plugin Entry Point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new strata::engine::StrataAudioProcessor();
}
