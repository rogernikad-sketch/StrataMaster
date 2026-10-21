#include "dsp/common/Constants.h"
#include "dsp/common/DecibelUtils.h"
#include "dsp/common/DenormalGuard.h"
#include "dsp/common/RingBuffer.h"
#include "dsp/common/SmoothedValue.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("DecibelUtils conversion accuracy", "[dsp][common]") {
    SECTION("Linear to Decibels") {
        REQUIRE(strata::dsp::gainToDecibels(1.0f) == Catch::Approx(0.0f).margin(0.001f));
        REQUIRE(strata::dsp::gainToDecibels(0.5f) == Catch::Approx(-6.0206f).margin(0.01f));
        REQUIRE(strata::dsp::gainToDecibels(2.0f) == Catch::Approx(6.0206f).margin(0.01f));
        REQUIRE(strata::dsp::gainToDecibels(0.0f) == Catch::Approx(strata::dsp::MIN_DB));
    }

    SECTION("Decibels to Linear") {
        REQUIRE(strata::dsp::decibelsToGain(0.0f) == Catch::Approx(1.0f).margin(0.001f));
        REQUIRE(strata::dsp::decibelsToGain(-6.0206f) == Catch::Approx(0.5f).margin(0.01f));
        REQUIRE(strata::dsp::decibelsToGain(6.0206f) == Catch::Approx(2.0f).margin(0.01f));
        REQUIRE(strata::dsp::decibelsToGain(-120.0f) == Catch::Approx(0.0f).margin(0.0001f));
    }
}

TEST_CASE("SmoothedValue linear smoothing behavior", "[dsp][common]") {
    strata::dsp::SmoothedValue<float> smoother(0.0f);
    smoother.setSmoothingType(strata::dsp::SmoothingType::Linear);
    smoother.reset(100.0, 1.0); // 100 Hz sample rate, 1.0 second ramp = 100 samples
    smoother.setTargetValue(1.0f);

    REQUIRE(smoother.isSmoothing());
    for (int i = 0; i < 50; ++i) {
        smoother.getNextValue();
    }
    REQUIRE(smoother.getCurrentValue() == Catch::Approx(0.5f).margin(0.02f));

    for (int i = 0; i < 50; ++i) {
        smoother.getNextValue();
    }
    REQUIRE(smoother.getCurrentValue() == Catch::Approx(1.0f).margin(0.001f));
    REQUIRE_FALSE(smoother.isSmoothing());
}

TEST_CASE("RingBuffer SPSC push and pop", "[dsp][common]") {
    strata::dsp::RingBuffer<float> ring(64);
    REQUIRE(ring.getAvailableForRead() == 0);
    REQUIRE(ring.getAvailableForWrite() >= 63);

    std::vector<float> input = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    size_t written = ring.write(input.data(), input.size());
    REQUIRE(written == 5);
    REQUIRE(ring.getAvailableForRead() == 5);

    std::vector<float> output(5, 0.0f);
    size_t readCount = ring.read(output.data(), output.size());
    REQUIRE(readCount == 5);
    REQUIRE(output == input);
    REQUIRE(ring.getAvailableForRead() == 0);
}

TEST_CASE("DenormalGuard RAII", "[dsp][common]") {
    {
        strata::dsp::DenormalGuard guard;
        // Operation under guard should not crash or throw
        float tiny = 1.0e-38f;
        tiny *= 0.5f;
        (void)tiny;
    }
    SUCCEED("Denormal guard executed cleanly");
}
