#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "engine/StrataAudioProcessor.h"
#include "engine/ParameterIDs.h"
#include <chrono>
#include <vector>
#include <numbers>

using namespace strata;
using namespace strata::engine;

TEST_CASE("Full Mastering DSP Chain Performance & Real-Time Budget Benchmark", "[performance][dsp]") {
    juce::ScopedJuceInitialiser_GUI guiInit;

    StrataAudioProcessor processor;
    const double sampleRate = 44100.0;
    const int blockSize = 512;
    processor.prepareToPlay(sampleRate, blockSize);

    // Enable every module in the chain to measure worst-case DSP CPU load
    auto setParam = [&](const juce::String& paramId, float value) {
        if (auto* p = processor.getAPVTS().getParameter(paramId)) {
            p->setValueNotifyingHost(p->convertTo0to1(value));
        }
    };

    setParam(juce::String(param::Bypass.data()), 0.0f);
    setParam(juce::String(param::EQBypass.data()), 0.0f);
    setParam(juce::String(param::DynamicsBypass.data()), 0.0f);
    setParam(juce::String(param::ExciterBypass.data()), 0.0f);
    setParam(juce::String(param::ExciterOversampling.data()), 2.0f); // 4x polyphase FIR oversampling
    setParam(juce::String(param::TransientBypass.data()), 0.0f);
    setParam(juce::String(param::StereoMonoMakerEnable.data()), 1.0f);
    setParam(juce::String(param::StereoStereoizerEnable.data()), 1.0f);
    setParam(juce::String(param::ClipperEnable.data()), 1.0f);
    setParam(juce::String(param::LimiterBypass.data()), 0.0f);

    // Warm-up pass (1 block)
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    for (int ch = 0; ch < 2; ++ch) {
        buffer.clear(ch, 0, blockSize);
    }
    processor.processBlock(buffer, midi);

    // Benchmark 10 seconds of complex multi-tone audio (441,000 samples, ~861 blocks)
    const int totalBlocks = static_cast<int>(10.0 * sampleRate / static_cast<double>(blockSize));
    constexpr double twoPi = 2.0 * std::numbers::pi_v<double>;

    double maxBlockTimeMs = 0.0;
    const auto startTime = std::chrono::high_resolution_clock::now();

    for (int b = 0; b < totalBlocks; ++b) {
        // Synthesize dynamic mix
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        for (int i = 0; i < blockSize; ++i) {
            const double t = static_cast<double>(b * blockSize + i) / sampleRate;
            const float s1 = 0.4f * std::sin(static_cast<float>(twoPi * 60.0 * t));
            const float s2 = 0.3f * std::sin(static_cast<float>(twoPi * 1000.0 * t));
            const float s3 = 0.2f * std::sin(static_cast<float>(twoPi * 8000.0 * t));
            left[i]  = s1 + s2 + s3;
            right[i] = s1 - s2 + s3;
        }

        const auto blockStart = std::chrono::high_resolution_clock::now();
        processor.processBlock(buffer, midi);
        const auto blockEnd = std::chrono::high_resolution_clock::now();

        const double blockTimeMs = std::chrono::duration<double, std::milli>(blockEnd - blockStart).count();
        if (blockTimeMs > maxBlockTimeMs) {
            maxBlockTimeMs = blockTimeMs;
        }
    }

    const auto endTime = std::chrono::high_resolution_clock::now();
    const double totalElapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    const double realTimeTargetMs = 10000.0;
    const double cpuLoadPercent = (totalElapsedMs / realTimeTargetMs) * 100.0;
    const double speedupFactor = realTimeTargetMs / totalElapsedMs;

    // Output benchmark metrics
    // Budget: 512 samples at 44.1 kHz is 11.61 ms.
    // Full DSP chain should consume less than 15% of real-time budget even under heavy load.
    REQUIRE(cpuLoadPercent < 25.0); // Real-time CPU usage under 25% for entire mastering chain
    REQUIRE(speedupFactor > 4.0);   // Processes at least 4x faster than real time
    REQUIRE(maxBlockTimeMs < 10.0); // No single block exceeds real-time buffer deadline (11.61 ms)
}

TEST_CASE("96 kHz High-Resolution Studio Processing Performance", "[performance][96khz]") {
    juce::ScopedJuceInitialiser_GUI guiInit;

    StrataAudioProcessor processor;
    const double sampleRate = 96000.0;
    const int blockSize = 1024;
    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;

    // 5 seconds at 96 kHz
    const int totalBlocks = static_cast<int>(5.0 * sampleRate / static_cast<double>(blockSize));
    const auto startTime = std::chrono::high_resolution_clock::now();

    for (int b = 0; b < totalBlocks; ++b) {
        for (int ch = 0; ch < 2; ++ch) {
            auto* p = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                p[i] = 0.25f;
            }
        }
        processor.processBlock(buffer, midi);
    }

    const auto endTime = std::chrono::high_resolution_clock::now();
    const double totalElapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    const double realTimeTargetMs = 5000.0;
    const double speedupFactor = realTimeTargetMs / totalElapsedMs;

    // Must still process faster than real time at 96 kHz
    REQUIRE(speedupFactor > 2.0);
}
