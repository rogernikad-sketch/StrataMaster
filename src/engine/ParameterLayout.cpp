#include "ParameterLayout.h"
#include "ParameterIDs.h"
#include "../dsp/DynamicEqualizer.h"
#include "../dsp/MultibandDynamics.h"

namespace strata::engine {

juce::AudioProcessorValueTreeState::ParameterLayout ParameterLayout::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master Input Gain (-24 dB to +24 dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::InputGain.data()), 1},
        "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Master Output Gain (-24 dB to +24 dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::OutputGain.data()), 1},
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Master Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{juce::String(param::Bypass.data()), 1},
        "Bypass",
        false
    ));

    // EQ Global Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{juce::String(param::EQBypass.data()), 1},
        "EQ Bypass",
        false
    ));

    // Dynamics Global Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{juce::String(param::DynamicsBypass.data()), 1},
        "Dynamics Bypass",
        false
    ));

    // Stereo Mode (Stereo, Mid/Side, Mid Solo, Side Solo)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{juce::String(param::StereoMode.data()), 1},
        "Stereo Mode",
        juce::StringArray{"Stereo", "Mid/Side", "Mid Solo", "Side Solo"},
        0
    ));

    // Stereo Width (0.0 = Mono, 1.0 = Normal, 2.0 = Wide)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::StereoWidth.data()), 1},
        "Stereo Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel("x")
    ));

    // Crossover Split Frequencies
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::CrossoverSplit1.data()), 1},
        "Crossover Split 1 (Low / Low-Mid)",
        juce::NormalisableRange<float>(40.0f, 500.0f, 1.0f, 0.4f),
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::CrossoverSplit2.data()), 1},
        "Crossover Split 2 (Low-Mid / High-Mid)",
        juce::NormalisableRange<float>(200.0f, 5000.0f, 1.0f, 0.4f),
        1500.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::CrossoverSplit3.data()), 1},
        "Crossover Split 3 (High-Mid / High)",
        juce::NormalisableRange<float>(1000.0f, 16000.0f, 1.0f, 0.4f),
        6000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    // 8-Band Parametric & Dynamic EQ Parameters
    const std::array<float, dsp::NUM_EQ_BANDS> defaultFreqs = {
        30.0f, 80.0f, 250.0f, 800.0f, 2000.0f, 5000.0f, 10000.0f, 16000.0f
    };
    const std::array<int, dsp::NUM_EQ_BANDS> defaultTypeIndices = {
        4, 1, 0, 0, 0, 0, 2, 3 // HP, LowShelf, Bell, Bell, Bell, Bell, HighShelf, LP
    };

    const juce::StringArray typeChoices{
        "Bell", "Low Shelf", "High Shelf", "Low Pass", "High Pass", "Notch", "Band Pass"
    };

    for (size_t b = 0; b < dsp::NUM_EQ_BANDS; ++b) {
        const juce::String bPrefix = "EQ Band " + juce::String(static_cast<int>(b + 1)) + " ";

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{param::getEQBandEnableID(b), 1},
            bPrefix + "Enable",
            (b > 0 && b < 7)
        ));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{param::getEQBandTypeID(b), 1},
            bPrefix + "Type",
            typeChoices,
            defaultTypeIndices[b]
        ));

        juce::NormalisableRange<float> freqRange(20.0f, 20000.0f, 0.1f, 0.25f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getEQBandFreqID(b), 1},
            bPrefix + "Frequency",
            freqRange,
            defaultFreqs[b],
            juce::AudioParameterFloatAttributes().withLabel("Hz")
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getEQBandGainID(b), 1},
            bPrefix + "Gain",
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")
        ));

        juce::NormalisableRange<float> qRange(0.1f, 10.0f, 0.01f, 0.4f);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getEQBandQID(b), 1},
            bPrefix + "Q",
            qRange,
            1.0f
        ));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{param::getEQBandDynEnableID(b), 1},
            bPrefix + "Dyn Enable",
            false
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getEQBandDynThreshID(b), 1},
            bPrefix + "Dyn Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
            -20.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getEQBandDynRangeID(b), 1},
            bPrefix + "Dyn Range",
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
            -6.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")
        ));
    }

    // 4-Band Multiband Dynamics Parameters
    const std::array<juce::String, dsp::NUM_CROSSOVER_BANDS> bandNames = {
        "Low", "Low-Mid", "High-Mid", "High"
    };
    const std::array<float, dsp::NUM_CROSSOVER_BANDS> defaultThreshs = {
        -16.0f, -18.0f, -20.0f, -22.0f
    };
    const std::array<float, dsp::NUM_CROSSOVER_BANDS> defaultAttacks = {
        30.0f, 20.0f, 12.0f, 8.0f
    };

    for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
        const juce::String bPrefix = "Dyn " + bandNames[b] + " ";

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{param::getDynBandEnableID(b), 1},
            bPrefix + "Enable",
            true
        ));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{param::getDynBandBypassID(b), 1},
            bPrefix + "Bypass",
            false
        ));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{param::getDynBandSoloID(b), 1},
            bPrefix + "Solo",
            false
        ));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{param::getDynBandMuteID(b), 1},
            bPrefix + "Mute",
            false
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getDynBandThreshID(b), 1},
            bPrefix + "Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
            defaultThreshs[b],
            juce::AudioParameterFloatAttributes().withLabel("dB")
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getDynBandRatioID(b), 1},
            bPrefix + "Ratio",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f),
            2.0f,
            juce::AudioParameterFloatAttributes().withLabel(":1")
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getDynBandAttackID(b), 1},
            bPrefix + "Attack",
            juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.35f),
            defaultAttacks[b],
            juce::AudioParameterFloatAttributes().withLabel("ms")
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getDynBandReleaseID(b), 1},
            bPrefix + "Release",
            juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.35f),
            100.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getDynBandMakeupID(b), 1},
            bPrefix + "Makeup",
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")
        ));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{param::getDynBandAutoMakeupID(b), 1},
            bPrefix + "Auto Makeup",
            false
        ));
    }

    // =========================================================================
    // Harmonic Exciter Parameters
    // =========================================================================
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{juce::String(param::ExciterBypass.data()), 1},
        "Exciter Bypass",
        false
    ));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{juce::String(param::ExciterOversampling.data()), 1},
        "Exciter Oversampling",
        juce::StringArray{"1x (Off)", "2x", "4x", "8x"},
        2 // Default 4x
    ));

    for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
        const juce::String bPrefix = "Exciter Band " + juce::String(b + 1) + " ";

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{param::getExciterBandEnableID(b), 1},
            bPrefix + "Enable",
            true
        ));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{param::getExciterBandModeID(b), 1},
            bPrefix + "Mode",
            juce::StringArray{"Tape", "Tube", "Warm", "Retro"},
            0
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getExciterBandDriveID(b), 1},
            bPrefix + "Drive",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getExciterBandMixID(b), 1},
            bPrefix + "Mix",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            100.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")
        ));
    }

    // =========================================================================
    // Transient & Sustain Processor Parameters
    // =========================================================================
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{juce::String(param::TransientBypass.data()), 1},
        "Transient Bypass",
        false
    ));

    for (size_t b = 0; b < dsp::NUM_CROSSOVER_BANDS; ++b) {
        const juce::String bPrefix = "Transient Band " + juce::String(b + 1) + " ";

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{param::getTransientBandEnableID(b), 1},
            bPrefix + "Enable",
            true
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getTransientBandAttackID(b), 1},
            bPrefix + "Attack",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{param::getTransientBandSustainID(b), 1},
            bPrefix + "Sustain",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")
        ));
    }

    // =========================================================================
    // Advanced Stereo Imager Parameters (Mono-maker & Stereoizer)
    // =========================================================================
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{juce::String(param::StereoMonoMakerEnable.data()), 1},
        "Mono Maker Enable",
        false
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::StereoMonoMakerFreq.data()), 1},
        "Mono Maker Frequency",
        juce::NormalisableRange<float>(20.0f, 400.0f, 1.0f, 0.4f),
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{juce::String(param::StereoStereoizerEnable.data()), 1},
        "Stereoizer Enable",
        false
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::StereoStereoizerAmount.data()), 1},
        "Stereoizer Amount",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // =========================================================================
    // Pre-Limiter Soft Clipper Parameters
    // =========================================================================
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{juce::String(param::ClipperEnable.data()), 1},
        "Clipper Enable",
        false
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::ClipperCeiling.data()), 1},
        "Clipper Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f),
        -0.5f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::ClipperKnee.data()), 1},
        "Clipper Knee",
        juce::NormalisableRange<float>(0.0f, 6.0f, 0.1f),
        2.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // =========================================================================
    // Lookahead True-Peak Limiter & Maximizer Parameters
    // =========================================================================
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{juce::String(param::LimiterBypass.data()), 1},
        "Limiter Bypass",
        false
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::LimiterCeiling.data()), 1},
        "Limiter Ceiling",
        juce::NormalisableRange<float>(-6.0f, 0.0f, 0.05f),
        -0.1f,
        juce::AudioParameterFloatAttributes().withLabel("dBTP")
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::LimiterThreshold.data()), 1},
        "Limiter Threshold",
        juce::NormalisableRange<float>(-24.0f, 0.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::LimiterRelease.data()), 1},
        "Limiter Release",
        juce::NormalisableRange<float>(1.0f, 500.0f, 1.0f, 0.35f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{juce::String(param::LimiterLookahead.data()), 1},
        "Limiter Lookahead",
        juce::NormalisableRange<float>(0.5f, 10.0f, 0.1f),
        3.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    return {params.begin(), params.end()};
}

} // namespace strata::engine
