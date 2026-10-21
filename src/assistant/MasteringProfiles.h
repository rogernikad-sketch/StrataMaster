#pragma once

#include <string>
#include <array>

namespace strata::assistant {

enum class GenreProfile {
    StreamingStandard, // -14.0 LUFS, -1.0 dBTP
    ClubEDM,           // -7.0 LUFS, -0.1 dBTP
    CDPop,             // -9.0 LUFS, -0.3 dBTP
    AcousticWarm,      // -16.0 LUFS, -1.0 dBTP
    HipHop             // -8.0 LUFS, -0.2 dBTP
};

struct TargetProfile {
    GenreProfile genre{ GenreProfile::StreamingStandard };
    std::string name;
    float targetIntegratedLufs{ -14.0f };
    float targetTruePeakCeilingDb{ -1.0f };
    float targetCrestFactorDb{ 8.0f };
    float targetStereoWidth{ 100.0f };
    float monoMakerFrequencyHz{ 80.0f };

    // Target spectral distribution relative to 1 kHz reference (in dB)
    // Bands: 32Hz (Sub), 64Hz (Bass), 250Hz (Low-Mid), 1kHz (Mid), 3kHz (Upper-Mid), 6kHz (Presence), 12kHz (Air), 16kHz (Brilliance)
    std::array<float, 8> targetSpectralCurveDb{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    static TargetProfile getProfile(GenreProfile genre) noexcept {
        switch (genre) {
            case GenreProfile::StreamingStandard:
                return {
                    GenreProfile::StreamingStandard,
                    "Streaming Standard (Spotify / Apple Music)",
                    -14.0f,
                    -1.0f,
                    9.0f,
                    105.0f,
                    80.0f,
                    { +2.0f, +1.5f, +0.5f, 0.0f, -0.5f, -1.0f, -2.0f, -3.5f }
                };
            case GenreProfile::ClubEDM:
                return {
                    GenreProfile::ClubEDM,
                    "Club / EDM Punch",
                    -7.0f,
                    -0.1f,
                    6.0f,
                    115.0f,
                    90.0f,
                    { +5.0f, +3.5f, +0.0f, 0.0f, +0.5f, +1.0f, +0.5f, -1.0f }
                };
            case GenreProfile::CDPop:
                return {
                    GenreProfile::CDPop,
                    "Modern CD & Pop Master",
                    -9.0f,
                    -0.3f,
                    7.5f,
                    110.0f,
                    85.0f,
                    { +3.0f, +2.0f, +0.5f, 0.0f, +0.0f, +1.2f, +1.5f, -1.0f }
                };
            case GenreProfile::AcousticWarm:
                return {
                    GenreProfile::AcousticWarm,
                    "Acoustic & Warm Natural",
                    -16.0f,
                    -1.0f,
                    11.0f,
                    100.0f,
                    60.0f,
                    { +1.0f, +1.0f, +1.0f, 0.0f, -1.0f, -2.0f, -3.0f, -5.0f }
                };
            case GenreProfile::HipHop:
                return {
                    GenreProfile::HipHop,
                    "Hip Hop & Trap Weight",
                    -8.0f,
                    -0.2f,
                    6.5f,
                    108.0f,
                    95.0f,
                    { +6.0f, +4.0f, -0.5f, 0.0f, -0.5f, +0.5f, +1.0f, -1.5f }
                };
        }
        return getProfile(GenreProfile::StreamingStandard);
    }
};

} // namespace strata::assistant
