# ==============================================================================
# Dependencies.cmake - Pinned JUCE and testing dependencies
# ==============================================================================

include(FetchContent)

# 1. JUCE 7.0.12 Resolution
if(NOT TARGET juce::juce_audio_processors)
    if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
        message(STATUS "[StrataMaster] Using local JUCE from JUCE_DIR: ${JUCE_DIR}")
        add_subdirectory("${JUCE_DIR}" JUCE EXCLUDE_FROM_ALL)
    elseif(EXISTS "C:/dev/JUCE/CMakeLists.txt")
        message(STATUS "[StrataMaster] Using local JUCE from default C:/dev/JUCE")
        add_subdirectory("C:/dev/JUCE" JUCE EXCLUDE_FROM_ALL)
    else()
        message(STATUS "[StrataMaster] Fetching JUCE 7.0.12 from GitHub...")
        FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG        7.0.12
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(JUCE)
    endif()
endif()

# 2. Catch2 v3.5.2 for Unit Testing
if(BUILD_TESTING)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)
endif()
