#pragma once

#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <span>
#include <juce_audio_processors/juce_audio_processors.h>

namespace strata::assistant {

enum class GenreProfile {
    // 35 Complete Reference Target Profiles
    AllPurpose = 0,
    AltRock,
    Ambient,
    ClassicHipHop,
    ClassicRock,
    Country,
    DancePop,
    DrumAndBass,
    Dubstep,
    EDM,
    Electropop,
    Folk,
    FutureBass,
    HipHop,
    House,
    Hyperpop,
    IndiePop,
    IndieRock,
    Jazz,
    KPopJPop,
    LatinPop,
    LoFi,
    Metal,
    Orchestral,
    Pop,
    PopCountry,
    PostRock,
    PunkRock,
    Reggae,
    Reggaeton,
    RnBSoul,
    Rock,
    Techno,
    Trap,
    VocalJazz,
    AcousticWarm,

    // Aliases for backward compatibility with earlier code & test fixtures
    StreamingStandard = AllPurpose,
    ClubEDM           = EDM,
    CDPop             = Pop,
    Classical         = Orchestral
};

struct TargetProfile {
    GenreProfile genre{ GenreProfile::AllPurpose };
    std::string name;
    std::string category;
    float targetIntegratedLufs{ -14.0f };
    float targetTruePeakCeilingDb{ -1.0f };
    float targetCrestFactorDb{ 8.0f };
    float targetStereoWidth{ 105.0f }; // In % (100% = 1.0x width)
    float monoMakerFrequencyHz{ 80.0f };

    // Target spectral distribution relative to 1 kHz reference (in dB)
    // Bands: 32Hz (Sub), 64Hz (Bass), 250Hz (Low-Mid), 1kHz (Mid), 3kHz (Upper-Mid), 6kHz (Presence), 12kHz (Air), 16kHz (Brilliance)
    std::array<float, 8> targetSpectralCurveDb{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    // Multiband Dynamics: per-band target threshold [dB] and ratio
    std::array<float, 4> dynThresholdsDb{ -16.0f, -18.0f, -20.0f, -22.0f };
    std::array<float, 4> dynRatios{ 2.0f, 2.0f, 2.0f, 2.0f };

    // Multiband Exciter: per-band mode (0=Tape, 1=Tube, 2=Warm, 3=Retro), drive [%], mix [%]
    std::array<int, 4> exciterModes{ 0, 0, 0, 0 };
    std::array<float, 4> exciterDrives{ 0.0f, 0.0f, 0.0f, 0.0f };

    // Multiband Transient: per-band attack [dB] and sustain [dB]
    std::array<float, 4> transientAttacks{ 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> transientSustains{ 0.0f, 0.0f, 0.0f, 0.0f };

    static const std::vector<TargetProfile>& getAllProfiles() noexcept {
        static const std::vector<TargetProfile> allProfiles = {
            // 1. All-Purpose (Universal)
            {
                GenreProfile::AllPurpose,
                "All-Purpose / Universal Reference",
                "Universal",
                -14.0f, -1.0f, 9.0f, 105.0f, 80.0f,
                { +2.0f, +1.5f, +0.5f, 0.0f, -0.5f, -1.0f, -2.0f, -3.5f },
                { -16.0f, -18.0f, -20.0f, -22.0f },
                { 1.8f, 1.8f, 1.8f, 1.5f },
                { 0, 0, 2, 0 },
                { 10.0f, 8.0f, 12.0f, 15.0f },
                { 0.5f, 0.0f, 0.5f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f }
            },
            // 2. Alt Rock
            {
                GenreProfile::AltRock,
                "Alt Rock & Grunge Edge",
                "Rock & Metal",
                -9.5f, -0.3f, 7.5f, 112.0f, 85.0f,
                { +2.5f, +2.5f, +1.5f, 0.0f, +1.5f, +2.0f, +0.5f, -1.5f },
                { -14.0f, -16.0f, -18.0f, -20.0f },
                { 2.2f, 2.2f, 2.0f, 1.8f },
                { 0, 2, 1, 0 },
                { 15.0f, 12.0f, 18.0f, 15.0f },
                { 1.5f, 1.0f, 1.5f, 0.5f },
                { 0.5f, 0.0f, 0.0f, -0.5f }
            },
            // 3. Ambient
            {
                GenreProfile::Ambient,
                "Ambient & Cinematic Immersion",
                "Acoustic & Classical",
                -16.0f, -1.0f, 11.0f, 130.0f, 75.0f,
                { +2.0f, +1.0f, 0.0f, 0.0f, -1.0f, -0.5f, +1.0f, 0.0f },
                { -18.0f, -20.0f, -22.0f, -24.0f },
                { 1.5f, 1.5f, 1.4f, 1.4f },
                { 2, 0, 2, 0 },
                { 12.0f, 10.0f, 15.0f, 15.0f },
                { -1.0f, -0.5f, 0.0f, 0.0f },
                { 1.5f, 1.5f, 1.0f, 1.0f }
            },
            // 4. Classic Hip-Hop
            {
                GenreProfile::ClassicHipHop,
                "Classic Hip-Hop & Boom Bap",
                "Hip-Hop & Urban",
                -9.0f, -0.3f, 7.5f, 106.0f, 95.0f,
                { +4.5f, +3.5f, +0.5f, 0.0f, +1.0f, +1.0f, -0.5f, -2.5f },
                { -13.0f, -15.0f, -17.0f, -19.0f },
                { 2.6f, 2.2f, 2.0f, 1.8f },
                { 0, 1, 0, 3 },
                { 20.0f, 18.0f, 15.0f, 16.0f },
                { 2.5f, 2.0f, 1.0f, 0.5f },
                { -0.5f, 0.0f, 0.0f, 0.0f }
            },
            // 5. Classic Rock
            {
                GenreProfile::ClassicRock,
                "Classic Rock & Vintage Warmth",
                "Rock & Metal",
                -10.5f, -0.5f, 8.5f, 110.0f, 80.0f,
                { +1.5f, +2.0f, +1.5f, 0.0f, +1.5f, +1.0f, -1.0f, -3.0f },
                { -15.0f, -17.0f, -19.0f, -21.0f },
                { 2.0f, 1.8f, 1.8f, 1.6f },
                { 0, 2, 0, 0 },
                { 18.0f, 15.0f, 12.0f, 10.0f },
                { 1.0f, 1.0f, 1.0f, 0.5f },
                { 0.5f, 0.5f, 0.0f, 0.0f }
            },
            // 6. Country
            {
                GenreProfile::Country,
                "Country & Acoustic Storytelling",
                "Acoustic & Classical",
                -13.0f, -0.8f, 10.0f, 106.0f, 75.0f,
                { +1.5f, +1.5f, +1.0f, 0.0f, +1.0f, +1.5f, +0.5f, -1.5f },
                { -16.0f, -18.0f, -20.0f, -22.0f },
                { 1.8f, 1.6f, 1.6f, 1.5f },
                { 2, 2, 0, 0 },
                { 12.0f, 10.0f, 12.0f, 14.0f },
                { 0.5f, 0.5f, 1.0f, 0.5f },
                { 0.5f, 0.5f, 0.0f, 0.0f }
            },
            // 7. Dance Pop
            {
                GenreProfile::DancePop,
                "Dance Pop & Commercial Polish",
                "Pop & Commercial",
                -8.5f, -0.2f, 6.8f, 115.0f, 100.0f,
                { +4.0f, +3.0f, +0.5f, 0.0f, +1.0f, +2.0f, +2.5f, +0.5f },
                { -12.0f, -14.0f, -16.0f, -18.0f },
                { 2.8f, 2.4f, 2.2f, 2.0f },
                { 0, 1, 3, 3 },
                { 18.0f, 15.0f, 20.0f, 24.0f },
                { 2.0f, 1.5f, 1.0f, 0.5f },
                { -0.5f, -0.5f, 0.0f, 0.0f }
            },
            // 8. Drum & Bass
            {
                GenreProfile::DrumAndBass,
                "Drum & Bass / Neurofunk",
                "Electronic & Dance",
                -6.5f, -0.1f, 5.5f, 118.0f, 120.0f,
                { +6.0f, +4.0f, -0.5f, 0.0f, +1.0f, +2.0f, +2.5f, +1.0f },
                { -9.0f, -11.0f, -13.0f, -15.0f },
                { 3.5f, 3.0f, 2.8f, 2.5f },
                { 1, 0, 3, 3 },
                { 25.0f, 20.0f, 28.0f, 30.0f },
                { 3.0f, 2.5f, 1.5f, 1.0f },
                { -1.5f, -1.0f, 0.0f, 0.5f }
            },
            // 9. Dubstep
            {
                GenreProfile::Dubstep,
                "Dubstep & Bass Heavy",
                "Electronic & Dance",
                -6.0f, -0.1f, 5.0f, 120.0f, 130.0f,
                { +6.5f, +4.5f, -1.0f, 0.0f, +1.5f, +2.5f, +3.0f, +1.5f },
                { -8.0f, -10.0f, -12.0f, -14.0f },
                { 3.8f, 3.2f, 3.0f, 2.8f },
                { 1, 1, 3, 3 },
                { 28.0f, 22.0f, 30.0f, 32.0f },
                { 3.2f, 2.8f, 1.8f, 1.0f },
                { -2.0f, -1.5f, 0.0f, 0.5f }
            },
            // 10. EDM (ClubEDM)
            {
                GenreProfile::EDM,
                "Club & Electronic Impact",
                "Electronic & Dance",
                -7.0f, -0.1f, 6.0f, 125.0f, 120.0f,
                { +5.0f, +3.5f, 0.0f, 0.0f, +0.5f, +1.5f, +2.0f, 0.0f },
                { -10.0f, -12.0f, -14.0f, -16.0f },
                { 3.2f, 2.8f, 2.5f, 2.2f },
                { 1, 0, 3, 3 },
                { 22.0f, 18.0f, 25.0f, 28.0f },
                { 2.5f, 2.0f, 1.0f, 0.5f },
                { -1.5f, -1.0f, 0.0f, 0.5f }
            },
            // 11. Electropop
            {
                GenreProfile::Electropop,
                "Electropop & Modern Synth",
                "Pop & Commercial",
                -8.5f, -0.2f, 7.0f, 114.0f, 95.0f,
                { +3.5f, +2.5f, +0.5f, 0.0f, +1.0f, +2.0f, +2.0f, +0.5f },
                { -13.0f, -15.0f, -17.0f, -19.0f },
                { 2.5f, 2.2f, 2.0f, 1.8f },
                { 0, 1, 3, 0 },
                { 16.0f, 14.0f, 20.0f, 22.0f },
                { 1.8f, 1.2f, 1.0f, 0.5f },
                { -0.5f, 0.0f, 0.0f, 0.0f }
            },
            // 12. Folk
            {
                GenreProfile::Folk,
                "Folk & Acoustic Detail",
                "Acoustic & Classical",
                -15.0f, -1.0f, 11.5f, 105.0f, 70.0f,
                { +1.0f, +1.5f, +1.5f, 0.0f, +0.5f, +1.0f, -1.0f, -3.0f },
                { -18.0f, -20.0f, -22.0f, -24.0f },
                { 1.5f, 1.5f, 1.4f, 1.4f },
                { 2, 2, 0, 0 },
                { 12.0f, 10.0f, 10.0f, 12.0f },
                { 0.0f, 0.5f, 1.0f, 0.5f },
                { 0.5f, 0.5f, 0.0f, 0.0f }
            },
            // 13. Future Bass
            {
                GenreProfile::FutureBass,
                "Future Bass & Lush Chords",
                "Electronic & Dance",
                -7.5f, -0.2f, 6.5f, 122.0f, 110.0f,
                { +5.5f, +3.5f, 0.0f, 0.0f, +1.0f, +2.0f, +2.5f, +1.0f },
                { -11.0f, -13.0f, -15.0f, -17.0f },
                { 3.0f, 2.6f, 2.4f, 2.0f },
                { 1, 0, 3, 3 },
                { 20.0f, 16.0f, 24.0f, 26.0f },
                { 2.2f, 1.8f, 1.2f, 0.5f },
                { -1.0f, -0.5f, 0.5f, 0.5f }
            },
            // 14. Hip-Hop
            {
                GenreProfile::HipHop,
                "Hip-Hop & Modern Trap Weight",
                "Hip-Hop & Urban",
                -8.0f, -0.2f, 6.5f, 110.0f, 110.0f,
                { +5.5f, +4.0f, -1.0f, 0.0f, +1.0f, +1.5f, +1.0f, -1.5f },
                { -12.0f, -14.0f, -16.0f, -18.0f },
                { 3.0f, 2.5f, 2.0f, 2.0f },
                { 1, 1, 0, 3 },
                { 25.0f, 20.0f, 15.0f, 20.0f },
                { 3.0f, 2.0f, 1.5f, 1.0f },
                { -1.0f, -0.5f, 0.0f, 0.0f }
            },
            // 15. House
            {
                GenreProfile::House,
                "House & Deep Club Groove",
                "Electronic & Dance",
                -8.0f, -0.2f, 6.5f, 118.0f, 105.0f,
                { +4.5f, +3.5f, +0.5f, 0.0f, +0.5f, +1.5f, +1.8f, 0.0f },
                { -11.0f, -13.0f, -15.0f, -17.0f },
                { 2.8f, 2.5f, 2.2f, 2.0f },
                { 0, 1, 0, 3 },
                { 18.0f, 16.0f, 18.0f, 20.0f },
                { 2.2f, 1.8f, 1.0f, 0.5f },
                { -1.0f, -0.5f, 0.0f, 0.0f }
            },
            // 16. Hyperpop
            {
                GenreProfile::Hyperpop,
                "Hyperpop & Glossy Sizzle",
                "Electronic & Dance",
                -7.0f, -0.1f, 5.5f, 122.0f, 115.0f,
                { +4.5f, +3.0f, -0.5f, 0.0f, +1.5f, +3.0f, +3.5f, +2.0f },
                { -9.0f, -11.0f, -13.0f, -15.0f },
                { 3.5f, 3.0f, 2.8f, 2.4f },
                { 3, 3, 3, 3 },
                { 25.0f, 22.0f, 30.0f, 35.0f },
                { 2.8f, 2.0f, 1.8f, 1.0f },
                { -1.5f, -1.0f, 0.0f, 0.0f }
            },
            // 17. Indie Pop
            {
                GenreProfile::IndiePop,
                "Indie Pop & Warm Jangle",
                "Pop & Commercial",
                -11.0f, -0.5f, 9.0f, 112.0f, 80.0f,
                { +2.0f, +2.0f, +1.0f, 0.0f, +1.0f, +1.5f, +0.5f, -1.0f },
                { -15.0f, -17.0f, -19.0f, -21.0f },
                { 2.0f, 1.8f, 1.8f, 1.6f },
                { 0, 2, 0, 0 },
                { 14.0f, 12.0f, 14.0f, 15.0f },
                { 1.0f, 0.8f, 0.8f, 0.5f },
                { 0.5f, 0.0f, 0.0f, 0.0f }
            },
            // 18. Indie Rock
            {
                GenreProfile::IndieRock,
                "Indie Rock & Dynamic Crunch",
                "Rock & Metal",
                -10.0f, -0.4f, 8.0f, 114.0f, 85.0f,
                { +2.0f, +2.5f, +1.5f, 0.0f, +1.8f, +1.5f, +0.5f, -1.5f },
                { -14.0f, -16.0f, -18.0f, -20.0f },
                { 2.2f, 2.0f, 2.0f, 1.8f },
                { 0, 1, 0, 0 },
                { 16.0f, 14.0f, 16.0f, 15.0f },
                { 1.4f, 1.2f, 1.2f, 0.8f },
                { 0.5f, 0.5f, 0.0f, 0.0f }
            },
            // 19. Jazz
            {
                GenreProfile::Jazz,
                "Jazz & Natural Warm Ensemble",
                "Acoustic & Classical",
                -14.5f, -1.0f, 11.0f, 105.0f, 60.0f,
                { +1.0f, +1.8f, +1.5f, 0.0f, -0.5f, +0.5f, -1.5f, -3.5f },
                { -18.0f, -20.0f, -22.0f, -24.0f },
                { 1.6f, 1.5f, 1.5f, 1.4f },
                { 2, 2, 1, 0 },
                { 14.0f, 12.0f, 10.0f, 8.0f },
                { 0.0f, 0.5f, 0.5f, 0.0f },
                { 0.5f, 0.5f, 0.0f, 0.0f }
            },
            // 20. K-Pop / J-Pop
            {
                GenreProfile::KPopJPop,
                "K-Pop / J-Pop High Energy",
                "Pop & Commercial",
                -8.0f, -0.2f, 6.5f, 116.0f, 100.0f,
                { +4.5f, +3.2f, +0.2f, 0.0f, +1.2f, +2.2f, +2.8f, +1.0f },
                { -11.0f, -13.0f, -15.0f, -17.0f },
                { 3.0f, 2.6f, 2.4f, 2.2f },
                { 0, 1, 3, 3 },
                { 20.0f, 18.0f, 24.0f, 26.0f },
                { 2.5f, 2.0f, 1.2f, 0.8f },
                { -0.8f, -0.4f, 0.0f, 0.0f }
            },
            // 21. Latin Pop
            {
                GenreProfile::LatinPop,
                "Latin Pop & Tropical Shine",
                "Pop & Commercial",
                -8.5f, -0.2f, 7.0f, 112.0f, 95.0f,
                { +4.0f, +3.0f, +0.5f, 0.0f, +1.0f, +1.8f, +2.0f, +0.5f },
                { -12.0f, -14.0f, -16.0f, -18.0f },
                { 2.6f, 2.3f, 2.0f, 1.8f },
                { 0, 2, 0, 3 },
                { 18.0f, 15.0f, 18.0f, 22.0f },
                { 2.2f, 1.6f, 1.0f, 0.5f },
                { -0.5f, 0.0f, 0.0f, 0.0f }
            },
            // 22. LoFi
            {
                GenreProfile::LoFi,
                "Lo-Fi & Vintage Warm Dust",
                "Hip-Hop & Urban",
                -13.5f, -0.8f, 10.0f, 102.0f, 70.0f,
                { +2.5f, +3.0f, +2.0f, 0.0f, -0.5f, -1.0f, -3.5f, -6.0f },
                { -16.0f, -18.0f, -20.0f, -22.0f },
                { 1.8f, 1.7f, 1.5f, 1.4f },
                { 0, 0, 3, 0 },
                { 22.0f, 18.0f, 15.0f, 12.0f },
                { 0.5f, 0.5f, 0.0f, -0.5f },
                { 1.0f, 1.0f, 0.5f, 0.0f }
            },
            // 23. Metal
            {
                GenreProfile::Metal,
                "Metal & Aggressive Wall",
                "Rock & Metal",
                -7.5f, -0.1f, 5.5f, 116.0f, 90.0f,
                { +3.5f, +3.0f, +0.5f, 0.0f, +2.2f, +2.5f, +1.5f, -0.5f },
                { -9.0f, -11.0f, -13.0f, -15.0f },
                { 3.2f, 2.8f, 2.6f, 2.2f },
                { 1, 1, 1, 0 },
                { 22.0f, 20.0f, 25.0f, 20.0f },
                { 2.8f, 2.2f, 1.8f, 1.0f },
                { -1.0f, -0.5f, 0.0f, 0.0f }
            },
            // 24. Orchestral (Classical)
            {
                GenreProfile::Orchestral,
                "Classical & Concert Hall Dynamics",
                "Acoustic & Classical",
                -18.0f, -1.5f, 14.0f, 110.0f, 45.0f,
                { +0.5f, +0.5f, +0.5f, 0.0f, -0.5f, -1.0f, -2.5f, -4.5f },
                { -24.0f, -26.0f, -28.0f, -30.0f },
                { 1.2f, 1.2f, 1.2f, 1.2f },
                { 2, 2, 2, 2 },
                { 0.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f }
            },
            // 25. Pop (CDPop)
            {
                GenreProfile::Pop,
                "Modern Pop & Polished Commercial",
                "Pop & Commercial",
                -9.0f, -0.3f, 7.5f, 112.0f, 85.0f,
                { +3.0f, +2.0f, +0.5f, 0.0f, +0.5f, +1.5f, +2.0f, -0.5f },
                { -14.0f, -16.0f, -18.0f, -20.0f },
                { 2.4f, 2.2f, 2.0f, 1.8f },
                { 0, 2, 1, 0 },
                { 16.0f, 14.0f, 18.0f, 22.0f },
                { 1.5f, 1.0f, 1.0f, 0.5f },
                { 0.0f, 0.0f, 0.0f, 0.0f }
            },
            // 26. Pop Country
            {
                GenreProfile::PopCountry,
                "Modern Pop Country",
                "Pop & Commercial",
                -10.0f, -0.4f, 8.5f, 108.0f, 80.0f,
                { +2.5f, +2.0f, +1.0f, 0.0f, +1.0f, +1.8f, +1.2f, -1.0f },
                { -14.0f, -16.0f, -18.0f, -20.0f },
                { 2.2f, 2.0f, 1.8f, 1.6f },
                { 0, 2, 0, 0 },
                { 15.0f, 12.0f, 15.0f, 18.0f },
                { 1.2f, 1.0f, 1.0f, 0.5f },
                { 0.5f, 0.5f, 0.0f, 0.0f }
            },
            // 27. Post Rock
            {
                GenreProfile::PostRock,
                "Post Rock & Vast Soundscapes",
                "Rock & Metal",
                -12.0f, -0.6f, 9.5f, 124.0f, 80.0f,
                { +2.0f, +2.0f, +1.0f, 0.0f, +1.0f, +1.2f, +1.5f, 0.0f },
                { -16.0f, -18.0f, -20.0f, -22.0f },
                { 2.0f, 1.8f, 1.8f, 1.6f },
                { 2, 0, 2, 0 },
                { 15.0f, 12.0f, 16.0f, 16.0f },
                { 0.8f, 0.8f, 0.8f, 0.5f },
                { 1.0f, 1.0f, 0.5f, 0.5f }
            },
            // 28. Punk Rock
            {
                GenreProfile::PunkRock,
                "Punk Rock & Raw Energy",
                "Rock & Metal",
                -8.5f, -0.2f, 6.5f, 112.0f, 85.0f,
                { +2.0f, +2.5f, +1.0f, 0.0f, +2.0f, +2.2f, +1.0f, -1.0f },
                { -12.0f, -14.0f, -16.0f, -18.0f },
                { 2.6f, 2.4f, 2.2f, 2.0f },
                { 1, 0, 1, 0 },
                { 20.0f, 18.0f, 22.0f, 18.0f },
                { 2.2f, 1.8f, 1.5f, 0.8f },
                { -0.5f, 0.0f, 0.0f, 0.0f }
            },
            // 29. Reggae
            {
                GenreProfile::Reggae,
                "Reggae & Dub Low End",
                "Hip-Hop & Urban",
                -11.0f, -0.5f, 9.0f, 108.0f, 90.0f,
                { +5.0f, +4.5f, +1.5f, 0.0f, -0.5f, +0.5f, 0.0f, -2.5f },
                { -14.0f, -16.0f, -18.0f, -20.0f },
                { 2.4f, 2.2f, 1.8f, 1.6f },
                { 0, 1, 0, 0 },
                { 18.0f, 16.0f, 12.0f, 10.0f },
                { 1.8f, 1.5f, 0.8f, 0.5f },
                { 0.5f, 0.5f, 0.0f, 0.0f }
            },
            // 30. Reggaeton
            {
                GenreProfile::Reggaeton,
                "Reggaeton & Dembow Punch",
                "Hip-Hop & Urban",
                -7.5f, -0.1f, 6.0f, 114.0f, 115.0f,
                { +5.5f, +4.0f, -0.5f, 0.0f, +1.0f, +1.8f, +1.5f, -0.5f },
                { -10.0f, -12.0f, -14.0f, -16.0f },
                { 3.2f, 2.8f, 2.4f, 2.0f },
                { 1, 0, 3, 3 },
                { 22.0f, 18.0f, 22.0f, 25.0f },
                { 3.0f, 2.4f, 1.2f, 0.8f },
                { -1.0f, -0.5f, 0.0f, 0.0f }
            },
            // 31. R&B / Soul
            {
                GenreProfile::RnBSoul,
                "R&B, Soul & Deep Velvet Groove",
                "Hip-Hop & Urban",
                -10.0f, -0.5f, 8.0f, 108.0f, 100.0f,
                { +4.5f, +3.0f, +1.0f, 0.0f, 0.0f, +1.0f, +1.0f, -1.5f },
                { -14.0f, -16.0f, -18.0f, -20.0f },
                { 2.5f, 2.2f, 2.0f, 1.8f },
                { 1, 2, 1, 0 },
                { 20.0f, 18.0f, 16.0f, 15.0f },
                { 2.0f, 1.0f, 0.5f, 0.0f },
                { 0.0f, 0.5f, 0.0f, 0.0f }
            },
            // 32. Rock
            {
                GenreProfile::Rock,
                "Rock & Driving Energy",
                "Rock & Metal",
                -9.5f, -0.3f, 7.5f, 115.0f, 90.0f,
                { +2.5f, +2.5f, +1.0f, 0.0f, +2.0f, +1.5f, 0.0f, -2.0f },
                { -14.0f, -16.0f, -18.0f, -20.0f },
                { 2.2f, 2.0f, 2.2f, 1.8f },
                { 0, 2, 1, 0 },
                { 15.0f, 15.0f, 20.0f, 18.0f },
                { 1.5f, 1.5f, 2.0f, 1.0f },
                { 0.5f, 0.5f, 0.0f, -0.5f }
            },
            // 33. Techno
            {
                GenreProfile::Techno,
                "Techno & Industrial Pulse",
                "Electronic & Dance",
                -7.0f, -0.1f, 5.8f, 120.0f, 125.0f,
                { +5.5f, +4.0f, -0.5f, 0.0f, +0.8f, +1.8f, +2.2f, +0.5f },
                { -10.0f, -12.0f, -14.0f, -16.0f },
                { 3.4f, 3.0f, 2.6f, 2.2f },
                { 1, 1, 3, 3 },
                { 24.0f, 20.0f, 26.0f, 28.0f },
                { 2.8f, 2.2f, 1.2f, 0.5f },
                { -1.5f, -1.0f, 0.0f, 0.5f }
            },
            // 34. Trap
            {
                GenreProfile::Trap,
                "Modern Trap Heavy Weight",
                "Hip-Hop & Urban",
                -7.5f, -0.1f, 5.8f, 112.0f, 120.0f,
                { +6.0f, +4.5f, -1.0f, 0.0f, +1.2f, +2.0f, +2.0f, +0.0f },
                { -10.0f, -12.0f, -14.0f, -16.0f },
                { 3.2f, 2.8f, 2.4f, 2.2f },
                { 1, 1, 0, 3 },
                { 26.0f, 22.0f, 18.0f, 24.0f },
                { 3.2f, 2.2f, 1.5f, 1.0f },
                { -1.2f, -0.8f, 0.0f, 0.0f }
            },
            // 35. Vocal Jazz
            {
                GenreProfile::VocalJazz,
                "Vocal Jazz & Intimate Room",
                "Acoustic & Classical",
                -15.0f, -1.0f, 12.0f, 104.0f, 65.0f,
                { +0.8f, +1.5f, +1.2f, 0.0f, +0.5f, +1.0f, -1.0f, -3.0f },
                { -18.0f, -20.0f, -22.0f, -24.0f },
                { 1.6f, 1.5f, 1.4f, 1.4f },
                { 2, 2, 0, 0 },
                { 12.0f, 10.0f, 10.0f, 10.0f },
                { 0.0f, 0.5f, 0.8f, 0.5f },
                { 0.5f, 0.5f, 0.0f, 0.0f }
            },
            // Acoustic Warm (Legacy / Specialized Profile)
            {
                GenreProfile::AcousticWarm,
                "Acoustic & Organic Resonance",
                "Acoustic & Classical",
                -16.0f, -1.0f, 12.0f, 104.0f, 60.0f,
                { +1.0f, +1.0f, +1.0f, 0.0f, -1.0f, -2.0f, -3.0f, -5.0f },
                { -20.0f, -22.0f, -24.0f, -26.0f },
                { 1.5f, 1.4f, 1.4f, 1.3f },
                { 2, 0, 0, 0 },
                { 10.0f, 8.0f, 6.0f, 8.0f },
                { 0.0f, 0.5f, 0.5f, 0.0f },
                { 0.5f, 0.5f, 0.0f, 0.0f }
            }
        };
        return allProfiles;
    }

    static TargetProfile getProfile(GenreProfile genre) noexcept {
        const auto& profiles = getAllProfiles();
        for (const auto& p : profiles) {
            if (p.genre == genre) {
                return p;
            }
        }
        return profiles[0];
    }
};

} // namespace strata::assistant
