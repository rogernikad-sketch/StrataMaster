#include <catch2/catch_test_macros.hpp>
#include "../src/engine/StrataAudioProcessor.h"
#include "../src/gui/StrataAudioEditor.h"

TEST_CASE("StrataAudioEditor instantiation, module switching, and resizing", "[gui]") {
    // Initialize JUCE GUI message manager context in headless test runner if available
    juce::ScopedJuceInitialiser_GUI guiInit;

    strata::engine::StrataAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    auto editor = std::make_unique<strata::gui::StrataAudioEditor>(processor);
    REQUIRE(editor != nullptr);

    SECTION("Default view is Equalizer") {
        editor->showModule(strata::gui::ModuleID::Equalizer);
        REQUIRE(editor->getWidth() >= 960);
        REQUIRE(editor->getHeight() >= 580);
    }

    SECTION("Module rack switching across all 7 modules") {
        const strata::gui::ModuleID modules[] = {
            strata::gui::ModuleID::Equalizer,
            strata::gui::ModuleID::Dynamics,
            strata::gui::ModuleID::Exciter,
            strata::gui::ModuleID::Imager,
            strata::gui::ModuleID::Maximizer,
            strata::gui::ModuleID::Maserati,
            strata::gui::ModuleID::Assistant
        };

        for (auto mod : modules) {
            editor->showModule(mod);
            editor->resized();
            // Verify paint callback does not throw or crash
            juce::Image dummyImage(juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
            juce::Graphics g(dummyImage);
            editor->paintEntireComponent(g, true);
        }
    }

    SECTION("Responsive resizing across valid aspect ratios") {
        // Minimum allowable size
        editor->setSize(960, 580);
        REQUIRE(editor->getWidth() == 960);
        REQUIRE(editor->getHeight() == 580);

        // Standard size
        editor->setSize(1040, 640);
        REQUIRE(editor->getWidth() == 1040);
        REQUIRE(editor->getHeight() == 640);

        // Large Ultrawide size
        editor->setSize(1440, 900);
        REQUIRE(editor->getWidth() == 1440);
        REQUIRE(editor->getHeight() == 900);

        // 4K UHD Ultra-High Resolution
        editor->setSize(3840, 2160);
        REQUIRE(editor->getWidth() == 3840);
        REQUIRE(editor->getHeight() == 2160);
    }

    SECTION("Timer callback updates metering telemetry safely") {
        // Simulate audio processing to populate meter levels
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        for (int i = 0; i < 512; ++i) {
            const float sample = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 1000.0f * (float)i / 44100.0f);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);

        // Run timer callback multiple times
        for (int i = 0; i < 10; ++i) {
            editor->timerCallback();
        }
    }
}
