cmake_minimum_required(VERSION 3.22)

project(StrataMaster VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to build unit tests
option(BUILD_TESTING "Build the automated testing suite" ON)

# Modular CMake helpers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)
include(Dependencies)

# Source files for StrataMaster Audio Engine & Plugin
set(STRATA_SOURCES
    src/dsp/common/Constants.h
    src/dsp/common/DecibelUtils.h
    src/dsp/common/DenormalGuard.h
    src/dsp/common/SmoothedValue.h
    src/dsp/common/RingBuffer.h
    src/dsp/StereoProcessor.h
    src/dsp/StereoProcessor.cpp
    src/dsp/BiquadFilter.h
    src/dsp/BiquadFilter.cpp
    src/dsp/DynamicEqualizer.h
    src/dsp/DynamicEqualizer.cpp
    src/dsp/CrossoverFilterbank.h
    src/dsp/CrossoverFilterbank.cpp
    src/dsp/MultibandDynamics.h
    src/dsp/MultibandDynamics.cpp
    src/dsp/Oversampler.h
    src/dsp/Oversampler.cpp
    src/dsp/Saturator.h
    src/dsp/Saturator.cpp
    src/dsp/TransientProcessor.h
    src/dsp/TransientProcessor.cpp
    src/dsp/SoftClipper.h
    src/dsp/TruePeakLimiter.h
    src/dsp/TruePeakLimiter.cpp
    src/engine/ParameterIDs.h
    src/engine/ParameterLayout.h
    src/engine/ParameterLayout.cpp
    src/engine/PresetManager.h
    src/engine/PresetManager.cpp
    src/engine/ReferencePlayer.h
    src/engine/ReferencePlayer.cpp
    src/engine/StrataAudioProcessor.h
    src/engine/StrataAudioProcessor.cpp
    src/analysis/CorrelationMeter.h
    src/analysis/CorrelationMeter.cpp
    src/analysis/BS1770LoudnessMeter.h
    src/analysis/BS1770LoudnessMeter.cpp
    src/analysis/TruePeakMeter.h
    src/analysis/TruePeakMeter.cpp
    src/analysis/SpectrumAnalyzer.h
    src/analysis/SpectrumAnalyzer.cpp
    src/analysis/AudioFeatureExtractor.h
    src/analysis/AudioFeatureExtractor.cpp
    src/assistant/MasteringProfiles.h
    src/assistant/AutoMasterEngine.h
    src/assistant/AutoMasterEngine.cpp
    src/assistant/ReferenceMatcher.h
    src/assistant/ReferenceMatcher.cpp
    src/gui/LookAndFeel.h
    src/gui/LookAndFeel.cpp
    src/gui/components/Knob.h
    src/gui/components/Knob.cpp
    src/gui/components/LevelMeter.h
    src/gui/components/LevelMeter.cpp
    src/gui/components/SpectrumDisplay.h
    src/gui/components/SpectrumDisplay.cpp
    src/gui/components/EQNodeGraph.h
    src/gui/components/EQNodeGraph.cpp
    src/gui/components/LoudnessMeterBar.h
    src/gui/components/LoudnessMeterBar.cpp
    src/gui/components/ModuleRackBar.h
    src/gui/components/ModuleRackBar.cpp
    src/gui/components/CrossoverGraph.h
    src/gui/components/CrossoverGraph.cpp
    src/gui/views/EqualizerView.h
    src/gui/views/EqualizerView.cpp
    src/gui/views/DynamicsView.h
    src/gui/views/DynamicsView.cpp
    src/gui/views/ExciterView.h
    src/gui/views/ExciterView.cpp
    src/gui/views/ImagerView.h
    src/gui/views/ImagerView.cpp
    src/gui/views/MaximizerView.h
    src/gui/views/MaximizerView.cpp
    src/gui/views/MasterAssistantView.h
    src/gui/views/MasterAssistantView.cpp
    src/gui/StrataAudioEditor.h
    src/gui/StrataAudioEditor.cpp
)

# Define JUCE Plugin Target (VST3 + Standalone)
juce_add_plugin(StrataMaster
    COMPANY_NAME "StrataAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE StAu
    PLUGIN_CODE StMa
    FORMATS VST3 Standalone
    PRODUCT_NAME "StrataMaster"
)

target_sources(StrataMaster PRIVATE ${STRATA_SOURCES})

target_include_directories(StrataMaster PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(StrataMaster PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_plugin_client
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
)

target_compile_definitions(StrataMaster PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

strata_set_compiler_warnings(StrataMaster)

# Automated unit tests
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
