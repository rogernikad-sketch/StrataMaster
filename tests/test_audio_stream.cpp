#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../src/engine/StrataAudioProcessor.h"
#include "../src/engine/ParameterIDs.h"
#include "../src/dsp/common/Constants.h"
#include <vector>
#include <cmath>

TEST_CASE("StrataAudioProcessor continuous audio stream and click-free processing", "[engine][stream]") {
    strata::engine::StrataAudioProcessor processor;
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr int numBlocks = 100;

    processor.prepareToPlay(sampleRate, blockSize);

    const int initialLatency = processor.getLatencySamples();
    REQUIRE(initialLatency > 0);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midiMessages;

    // Continuous 1 kHz sine wave generator state
    double phase = 0.0;
    const double phaseIncrement = strata::dsp::TWO_PI * 1000.0 / sampleRate;

    float prevLastSampleL = 0.0f;
    float prevLastSampleR = 0.0f;
    bool hasPreviousBlock = false;

    // Track total blocks with non-zero audio
    int nonSilentBlocks = 0;
    float maxBoundaryDiscontinuity = 0.0f;

    for (int block = 0; block < numBlocks; ++block) {
        // Synthesize continuous sine wave block
        for (int i = 0; i < blockSize; ++i) {
            const float s = static_cast<float>(std::sin(phase) * 0.5); // -6 dBFS sine
            buffer.setSample(0, i, s);
            buffer.setSample(1, i, s);
            phase += phaseIncrement;
            if (phase >= strata::dsp::TWO_PI) {
                phase -= strata::dsp::TWO_PI;
            }
        }

        processor.processBlock(buffer, midiMessages);

        // 1. Verify latency does NOT ping-pong or change across steady-state blocks
        REQUIRE(processor.getLatencySamples() == initialLatency);

        const float magL = buffer.getMagnitude(0, 0, blockSize);
        const float magR = buffer.getMagnitude(1, 0, blockSize);

        // After initial lookahead delay (~147 samples < 1 block), output must be non-silent!
        if (block > 1) {
            CHECK(magL > 0.05f);
            CHECK(magR > 0.05f);
            if (magL > 0.05f && magR > 0.05f) {
                ++nonSilentBlocks;
            }

            // 2. Check boundary sample continuity between consecutive blocks (no tick/click)
            if (hasPreviousBlock) {
                const float diffL = std::abs(buffer.getSample(0, 0) - prevLastSampleL);
                const float diffR = std::abs(buffer.getSample(1, 0) - prevLastSampleR);
                maxBoundaryDiscontinuity = std::max(maxBoundaryDiscontinuity, std::max(diffL, diffR));

                // Maximum step between consecutive samples of a 1 kHz sine at 44.1 kHz is ~2*pi*1000/44100 * 0.5 ≈ 0.071
                // Any boundary step > 0.35 indicates a buffer discontinuity / click!
                CHECK(diffL < 0.25f);
                CHECK(diffR < 0.25f);
            }
        }

        prevLastSampleL = buffer.getSample(0, blockSize - 1);
        prevLastSampleR = buffer.getSample(1, blockSize - 1);
        hasPreviousBlock = true;
    }

    CHECK(nonSilentBlocks == numBlocks - 2);
    CHECK(maxBoundaryDiscontinuity < 0.25f);

    processor.releaseResources();
}

TEST_CASE("StrataAudioProcessor latency updates only on actual parameter modification", "[engine][latency]") {
    strata::engine::StrataAudioProcessor processor;
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    processor.prepareToPlay(sampleRate, blockSize);

    const int initialLatency = processor.getLatencySamples();
    REQUIRE(initialLatency > 0);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midiMessages;

    // Run 10 steady-state blocks - latency must remain constant
    for (int b = 0; b < 10; ++b) {
        buffer.clear();
        processor.processBlock(buffer, midiMessages);
        CHECK(processor.getLatencySamples() == initialLatency);
    }

    // Now modify exciter oversampling parameter to 8x (index 3)
    auto& apvts = *const_cast<juce::AudioProcessorValueTreeState*>(&processor.getAPVTS());
    auto* oversamplingParam = apvts.getParameter(juce::String(strata::param::ExciterOversampling.data()));
    REQUIRE(oversamplingParam != nullptr);

    // Change to 8x oversampling (index 3 out of 3, normalized 1.0)
    oversamplingParam->setValueNotifyingHost(1.0f);

    // Process block: latency should now reflect new 8x oversampling latency
    buffer.clear();
    processor.processBlock(buffer, midiMessages);
    const int newLatency = processor.getLatencySamples();
    CHECK(newLatency > initialLatency);

    // Subsequent blocks must keep the new latency stable without re-triggering
    for (int b = 0; b < 10; ++b) {
        buffer.clear();
        processor.processBlock(buffer, midiMessages);
        CHECK(processor.getLatencySamples() == newLatency);
    }

    processor.releaseResources();
}

TEST_CASE("StrataAudioProcessor bypass toggle maintains continuous audio output", "[engine][bypass]") {
    strata::engine::StrataAudioProcessor processor;
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midiMessages;

    auto& apvts = *const_cast<juce::AudioProcessorValueTreeState*>(&processor.getAPVTS());
    auto* bypassParam = apvts.getParameter(juce::String(strata::param::Bypass.data()));
    REQUIRE(bypassParam != nullptr);

    double phase = 0.0;
    const double phaseIncrement = strata::dsp::TWO_PI * 440.0 / sampleRate;

    // Helper to feed sine and process
    auto feedAndProcess = [&](bool /*expectedBypass*/) {
        for (int i = 0; i < blockSize; ++i) {
            const float s = static_cast<float>(std::sin(phase) * 0.5);
            buffer.setSample(0, i, s);
            buffer.setSample(1, i, s);
            phase += phaseIncrement;
            if (phase >= strata::dsp::TWO_PI) {
                phase -= strata::dsp::TWO_PI;
            }
        }
        processor.processBlock(buffer, midiMessages);
    };

    // 1. Active mode: run 10 blocks
    bypassParam->setValueNotifyingHost(0.0f);
    for (int b = 0; b < 10; ++b) {
        feedAndProcess(false);
    }
    CHECK(buffer.getMagnitude(0, 0, blockSize) > 0.05f);

    // 2. Bypass mode: run 10 blocks
    bypassParam->setValueNotifyingHost(1.0f);
    for (int b = 0; b < 10; ++b) {
        feedAndProcess(true);
    }
    CHECK(buffer.getMagnitude(0, 0, blockSize) > 0.05f);

    // 3. Return to active mode: run 10 blocks
    bypassParam->setValueNotifyingHost(0.0f);
    for (int b = 0; b < 10; ++b) {
        feedAndProcess(false);
    }
    CHECK(buffer.getMagnitude(0, 0, blockSize) > 0.05f);

    processor.releaseResources();
}

TEST_CASE("StrataAudioProcessor stereo width smoother responds within 2 blocks", "[engine][stereo]") {
    strata::engine::StrataAudioProcessor processor;
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midiMessages;

    auto& apvts = *const_cast<juce::AudioProcessorValueTreeState*>(&processor.getAPVTS());
    auto* widthParam = apvts.getParameter(juce::String(strata::param::StereoWidth.data()));
    REQUIRE(widthParam != nullptr);

    // Feed stereo audio: Left has signal, Right has 0
    auto feedStereo = [&]() {
        for (int i = 0; i < blockSize; ++i) {
            buffer.setSample(0, i, 0.5f);
            buffer.setSample(1, i, 0.0f);
        }
        processor.processBlock(buffer, midiMessages);
    };

    // Set width to 0.0 (Mono)
    widthParam->setValueNotifyingHost(widthParam->convertTo0to1(0.0f));

    // Process 3 blocks
    for (int b = 0; b < 3; ++b) {
        feedStereo();
    }

    // In mono, Left and Right must be virtually identical
    const float lastL = buffer.getSample(0, blockSize - 1);
    const float lastR = buffer.getSample(1, blockSize - 1);
    REQUIRE_THAT(lastL, Catch::Matchers::WithinAbs(lastR, 0.01f));

    processor.releaseResources();
}

TEST_CASE("StrataAudioProcessor genre defaults correctly configure all mastering stages", "[engine][genres]") {
    strata::engine::StrataAudioProcessor processor;
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    processor.prepareToPlay(sampleRate, blockSize);
    auto& apvts = *const_cast<juce::AudioProcessorValueTreeState*>(&processor.getAPVTS());
    auto& engine = processor.getAutoMasterEngine();

    // 1. Test Hip-Hop Genre Defaults
    engine.setTargetGenre(strata::assistant::GenreProfile::HipHop);
    engine.applyGenreDefaults(apvts);

    auto* mmFreq = apvts.getRawParameterValue(strata::param::StereoMonoMakerFreq.data());
    auto* limCeil = apvts.getRawParameterValue(strata::param::LimiterCeiling.data());
    auto* excDrive0 = apvts.getRawParameterValue(strata::param::getExciterBandDriveID(0));

    REQUIRE(mmFreq != nullptr);
    REQUIRE(limCeil != nullptr);
    REQUIRE(excDrive0 != nullptr);

    REQUIRE(mmFreq->load() == Catch::Approx(110.0f));
    REQUIRE(limCeil->load() == Catch::Approx(-0.2f));
    REQUIRE(excDrive0->load() == Catch::Approx(25.0f));

    // 2. Test Classical Genre Defaults
    engine.setTargetGenre(strata::assistant::GenreProfile::Classical);
    engine.applyGenreDefaults(apvts);

    REQUIRE(mmFreq->load() == Catch::Approx(45.0f));
    REQUIRE(limCeil->load() == Catch::Approx(-1.5f));
    REQUIRE(excDrive0->load() == Catch::Approx(0.0f));

    // 3. Test EDM Genre Defaults
    engine.setTargetGenre(strata::assistant::GenreProfile::ClubEDM);
    engine.applyGenreDefaults(apvts);

    REQUIRE(mmFreq->load() == Catch::Approx(120.0f));
    REQUIRE(limCeil->load() == Catch::Approx(-0.1f));

    processor.releaseResources();
}
