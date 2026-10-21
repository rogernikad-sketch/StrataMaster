# ==============================================================================
# tests/CMakeLists.txt - Automated C++ test suite using Catch2
# ==============================================================================

if(NOT TARGET Catch2::Catch2WithMain)
    return()
endif()

set(TEST_SOURCES
    test_smoothed_value.cpp
    test_stereo.cpp
    test_biquad.cpp
    test_crossover.cpp
    test_dynamics.cpp
    test_oversampler.cpp
    test_saturator.cpp
    test_transient.cpp
    test_limiter.cpp
    test_bs1770.cpp
    test_preset.cpp
    test_assistant.cpp
    test_performance.cpp
    ../src/dsp/StereoProcessor.cpp
    ../src/dsp/BiquadFilter.cpp
    ../src/dsp/DynamicEqualizer.cpp
    ../src/dsp/CrossoverFilterbank.cpp
    ../src/dsp/MultibandDynamics.cpp
    ../src/dsp/Oversampler.cpp
    ../src/dsp/Saturator.cpp
    ../src/dsp/TransientProcessor.cpp
    ../src/dsp/TruePeakLimiter.cpp
    ../src/analysis/BS1770LoudnessMeter.cpp
    ../src/analysis/TruePeakMeter.cpp
    ../src/analysis/SpectrumAnalyzer.cpp
    ../src/analysis/AudioFeatureExtractor.cpp
    ../src/analysis/CorrelationMeter.cpp
    ../src/engine/ParameterLayout.cpp
    ../src/engine/PresetManager.cpp
    ../src/engine/ReferencePlayer.cpp
    ../src/engine/StrataAudioProcessor.cpp
    ../src/assistant/AutoMasterEngine.cpp
    ../src/assistant/ReferenceMatcher.cpp
    ../src/gui/LookAndFeel.cpp
    ../src/gui/components/Knob.cpp
    ../src/gui/components/LevelMeter.cpp
    ../src/gui/components/SpectrumDisplay.cpp
    ../src/gui/components/EQNodeGraph.cpp
    ../src/gui/components/LoudnessMeterBar.cpp
    ../src/gui/StrataAudioEditor.cpp
)

add_executable(StrataTests ${TEST_SOURCES})

target_include_directories(StrataTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(StrataTests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_gui_basics
    juce::juce_dsp
)

target_compile_definitions(StrataTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_compile_features(StrataTests PRIVATE cxx_std_20)

strata_set_compiler_warnings(StrataTests)

include(CTest)
include(Catch)
catch_discover_tests(StrataTests)
