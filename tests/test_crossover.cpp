#include "dsp/CrossoverFilterbank.h"
#include "dsp/common/DecibelUtils.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>
#include <cmath>

TEST_CASE("CrossoverFilterbank flat all-pass summation across spectrum", "[dsp][crossover]") {
    const double sampleRate = 48000.0;
    strata::dsp::CrossoverFilterbank crossover;
    crossover.prepare(sampleRate);
    crossover.setCrossoverFrequencies(120.0f, 1500.0f, 6000.0f, true);

    const size_t numSamples = 4096;
    std::vector<float> impulse(numSamples, 0.0f);
    impulse[0] = 1.0f; // Dirac delta impulse

    std::vector<std::vector<float>> bands(4, std::vector<float>(numSamples, 0.0f));
    std::vector<float*> bandPointers = { bands[0].data(), bands[1].data(), bands[2].data(), bands[3].data() };
    std::vector<float> summed(numSamples, 0.0f);

    crossover.splitChannel(0, impulse.data(), bandPointers.data(), numSamples);
    strata::dsp::CrossoverFilterbank::sumChannel(bandPointers.data(), summed.data(), numSamples);

    // Compute DFT of the summed impulse response at key frequencies across 20 Hz - 20 kHz
    const std::vector<float> testFreqs = {
        20.0f, 40.0f, 80.0f, 120.0f, 250.0f, 800.0f, 1500.0f, 3000.0f, 6000.0f, 10000.0f, 16000.0f, 20000.0f
    };

    for (float freq : testFreqs) {
        const double omega = 2.0 * strata::dsp::PI * freq / sampleRate;
        double realSum = 0.0;
        double imagSum = 0.0;

        for (size_t n = 0; n < numSamples; ++n) {
            const double angle = omega * static_cast<double>(n);
            const double sample = static_cast<double>(summed[n]);
            realSum += sample * std::cos(angle);
            imagSum -= sample * std::sin(angle);
        }

        const double mag = std::sqrt(realSum * realSum + imagSum * imagSum);
        const double gainDb = 20.0 * std::log10(mag);

        // Summed frequency response must be flat within 0.05 dB of unity (0.0 dB)
        REQUIRE(gainDb == Catch::Approx(0.0).margin(0.05));
    }
}

TEST_CASE("CrossoverFilterbank steep band isolation", "[dsp][crossover]") {
    const double sampleRate = 48000.0;
    strata::dsp::CrossoverFilterbank crossover;
    crossover.prepare(sampleRate);
    crossover.setCrossoverFrequencies(120.0f, 1500.0f, 6000.0f, true);

    const size_t numSamples = 2048;
    std::vector<float> input(numSamples);
    std::vector<std::vector<float>> bands(4, std::vector<float>(numSamples, 0.0f));
    std::vector<float*> bandPointers = { bands[0].data(), bands[1].data(), bands[2].data(), bands[3].data() };

    SECTION("Low band receives 50 Hz and rejects 10 kHz") {
        for (size_t i = 0; i < numSamples; ++i) {
            input[i] = std::sin(2.0f * strata::dsp::PI * 50.0f * i / static_cast<float>(sampleRate));
        }
        crossover.splitChannel(0, input.data(), bandPointers.data(), numSamples);

        float peakLow = 0.0f;
        for (size_t i = 1024; i < numSamples; ++i) {
            peakLow = std::max(peakLow, std::abs(bands[0][i]));
        }
        REQUIRE(peakLow > 0.95f); // Full passband

        for (size_t i = 0; i < numSamples; ++i) {
            input[i] = std::sin(2.0f * strata::dsp::PI * 10000.0f * i / static_cast<float>(sampleRate));
        }
        crossover.splitChannel(0, input.data(), bandPointers.data(), numSamples);

        float peakHighLeakage = 0.0f;
        for (size_t i = 1024; i < numSamples; ++i) {
            peakHighLeakage = std::max(peakHighLeakage, std::abs(bands[0][i]));
        }
        REQUIRE(strata::dsp::gainToDecibels(peakHighLeakage) < -40.0f); // Steep stopband
    }
}
