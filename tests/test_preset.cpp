#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "engine/PresetManager.h"
#include "engine/ReferencePlayer.h"
#include "engine/ParameterIDs.h"
#include "dsp/common/DecibelUtils.h"
#include <juce_audio_processors/juce_audio_processors.h>

using namespace strata;
using namespace strata::engine;

namespace {

// Minimal dummy processor for testing APVTS and PresetManager in headless unit tests
class DummyProcessor : public juce::AudioProcessor {
public:
    DummyProcessor()
        : AudioProcessor(BusesProperties()
                             .withInput("Input", juce::AudioChannelSet::stereo(), true)
                             .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "DummyProcessor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

} // namespace

TEST_CASE("PresetManager Factory Presets and State Export", "[engine][preset]") {
    juce::ScopedJuceInitialiser_GUI guiInit;

    DummyProcessor processor;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(juce::String(param::InputGain.data()), 1),
        "Input Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f),
        0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(juce::String(param::StereoMonoMakerFreq.data()), 1),
        "Mono Maker Freq",
        juce::NormalisableRange<float>(20.0f, 400.0f),
        80.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(juce::String(param::LimiterCeiling.data()), 1),
        "Limiter Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f),
        -0.1f));

    juce::AudioProcessorValueTreeState apvts(processor, nullptr, "Parameters", std::move(layout));
    PresetManager presetManager(apvts);

    // 1. Verify Factory Presets Exist
    const auto& factoryPresets = presetManager.getFactoryPresets();
    REQUIRE(factoryPresets.size() >= 5);
    REQUIRE(factoryPresets[0].name == "Clean Master");
    REQUIRE(factoryPresets[1].name == "EDM Punch & Weight");

    // 2. Load EDM Preset
    presetManager.loadFactoryPreset("EDM Punch & Weight");
    auto* monoMakerFreqParam = apvts.getRawParameterValue(param::StereoMonoMakerFreq.data());
    REQUIRE(monoMakerFreqParam != nullptr);
    REQUIRE(monoMakerFreqParam->load() == Catch::Approx(90.0f).margin(0.5f));

    // 3. Load Broadcast Preset
    presetManager.loadFactoryPreset("Broadcast R128 (-1.0 dBTP)");
    auto* limiterCeilingParam = apvts.getRawParameterValue(param::LimiterCeiling.data());
    REQUIRE(limiterCeilingParam != nullptr);
    REQUIRE(limiterCeilingParam->load() == Catch::Approx(-1.0f).margin(0.1f));

    // 4. Test XML Round-Trip
    const auto xml = presetManager.exportCurrentStateToXml();
    REQUIRE(xml.isNotEmpty());
    REQUIRE(presetManager.restoreStateFromXml(xml) == true);
}

TEST_CASE("ReferencePlayer A/B Crossfading & Gain Matching", "[engine][reference]") {
    juce::ScopedJuceInitialiser_GUI guiInit;

    const double sampleRate = 44100.0;
    ReferencePlayer player;
    player.prepare(sampleRate, 512);

    // Create a 1-second reference buffer filled with constant 0.5 amplitude
    const int refLength = 44100;
    juce::AudioBuffer<float> refBuffer(2, refLength);
    for (int ch = 0; ch < 2; ++ch) {
        auto* writePtr = refBuffer.getWritePointer(ch);
        for (int i = 0; i < refLength; ++i) {
            writePtr[i] = 0.5f;
        }
    }

    player.loadDirectBuffer(std::move(refBuffer), sampleRate);
    REQUIRE(player.hasReferenceLoaded() == true);
    REQUIRE(player.getReferenceLengthSeconds() == Catch::Approx(1.0).margin(0.01));

    // Test A/B Crossfade Mode A (Default: Master track passed unchanged)
    const size_t blockSize = 512;
    std::vector<float> left(blockSize, 1.0f);
    std::vector<float> right(blockSize, 1.0f);

    player.setABMode(ABMode::TrackA);
    player.process(left.data(), right.data(), blockSize);

    // Left and right should still be ~1.0f (Track A)
    REQUIRE(left[0] == Catch::Approx(1.0f).margin(0.01f));
    REQUIRE(right[0] == Catch::Approx(1.0f).margin(0.01f));

    // Switch to Track B (Reference Track)
    player.setABMode(ABMode::TrackB);

    // Process enough samples for the 20 ms crossfade to complete (~1000 samples)
    for (int b = 0; b < 3; ++b) {
        player.process(left.data(), right.data(), blockSize);
    }

    // Now signal should match Reference Buffer (0.5f)
    REQUIRE(left[blockSize - 1] == Catch::Approx(0.5f).margin(0.02f));
    REQUIRE(right[blockSize - 1] == Catch::Approx(0.5f).margin(0.02f));

    // Test Gain Match Offset Calculation
    // Master is -14 LUFS, Reference is -10 LUFS -> Offset should be -4 dB
    const float offset = ReferencePlayer::computeGainMatchOffset(-14.0f, -10.0f);
    REQUIRE(offset == Catch::Approx(-4.0f).margin(0.01f));

    // Apply -6 dB gain match to reference
    player.setGainMatchEnabled(true);
    player.setGainMatchOffsetDb(-6.0206f); // factor of 0.5

    // Settle gain smoother
    for (int b = 0; b < 3; ++b) {
        player.process(left.data(), right.data(), blockSize);
    }

    // Reference (0.5f) * -6 dB (0.5x) = 0.25f
    REQUIRE(left[blockSize - 1] == Catch::Approx(0.25f).margin(0.03f));
    REQUIRE(right[blockSize - 1] == Catch::Approx(0.25f).margin(0.03f));
}
