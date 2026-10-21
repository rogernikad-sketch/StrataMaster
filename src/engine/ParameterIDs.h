#pragma once

#include <string>
#include <string_view>

namespace strata::param {

// Global Input & Output
inline constexpr std::string_view InputGain   = "input_gain";
inline constexpr std::string_view OutputGain  = "output_gain";
inline constexpr std::string_view Bypass      = "bypass";
inline constexpr std::string_view StereoMode  = "stereo_mode";
inline constexpr std::string_view StereoWidth = "stereo_width";

// Stereo Imager Advanced (Mono-maker & Stereoizer)
inline constexpr std::string_view StereoMonoMakerEnable  = "stereo_monomaker_enable";
inline constexpr std::string_view StereoMonoMakerFreq    = "stereo_monomaker_freq";
inline constexpr std::string_view StereoStereoizerEnable = "stereo_stereoizer_enable";
inline constexpr std::string_view StereoStereoizerAmount = "stereo_stereoizer_amount";

// Equalizer Global
inline constexpr std::string_view EQBypass    = "eq_bypass";

// EQ Band Helpers
inline std::string getEQBandEnableID(size_t band) { return "eq_b" + std::to_string(band) + "_enable"; }
inline std::string getEQBandTypeID(size_t band)   { return "eq_b" + std::to_string(band) + "_type"; }
inline std::string getEQBandFreqID(size_t band)   { return "eq_b" + std::to_string(band) + "_freq"; }
inline std::string getEQBandGainID(size_t band)   { return "eq_b" + std::to_string(band) + "_gain"; }
inline std::string getEQBandQID(size_t band)      { return "eq_b" + std::to_string(band) + "_q"; }
inline std::string getEQBandDynEnableID(size_t band) { return "eq_b" + std::to_string(band) + "_dyn_enable"; }
inline std::string getEQBandDynThreshID(size_t band) { return "eq_b" + std::to_string(band) + "_dyn_thresh"; }
inline std::string getEQBandDynRangeID(size_t band)  { return "eq_b" + std::to_string(band) + "_dyn_range"; }

// Multiband Dynamics Global & Crossover
inline constexpr std::string_view DynamicsBypass = "dynamics_bypass";
inline constexpr std::string_view CrossoverSplit1 = "crossover_split1";
inline constexpr std::string_view CrossoverSplit2 = "crossover_split2";
inline constexpr std::string_view CrossoverSplit3 = "crossover_split3";

// Multiband Dynamics Band Helpers
inline std::string getDynBandEnableID(size_t band)    { return "dyn_b" + std::to_string(band) + "_enable"; }
inline std::string getDynBandBypassID(size_t band)    { return "dyn_b" + std::to_string(band) + "_bypass"; }
inline std::string getDynBandSoloID(size_t band)      { return "dyn_b" + std::to_string(band) + "_solo"; }
inline std::string getDynBandMuteID(size_t band)      { return "dyn_b" + std::to_string(band) + "_mute"; }
inline std::string getDynBandThreshID(size_t band)    { return "dyn_b" + std::to_string(band) + "_thresh"; }
inline std::string getDynBandRatioID(size_t band)     { return "dyn_b" + std::to_string(band) + "_ratio"; }
inline std::string getDynBandAttackID(size_t band)    { return "dyn_b" + std::to_string(band) + "_attack"; }
inline std::string getDynBandReleaseID(size_t band)   { return "dyn_b" + std::to_string(band) + "_release"; }
inline std::string getDynBandMakeupID(size_t band)    { return "dyn_b" + std::to_string(band) + "_makeup"; }
inline std::string getDynBandAutoMakeupID(size_t band){ return "dyn_b" + std::to_string(band) + "_automakeup"; }

// Harmonic Exciter Global & Band Helpers
inline constexpr std::string_view ExciterBypass       = "exciter_bypass";
inline constexpr std::string_view ExciterOversampling = "exciter_oversampling";
inline std::string getExciterBandEnableID(size_t band) { return "exc_b" + std::to_string(band) + "_enable"; }
inline std::string getExciterBandModeID(size_t band)   { return "exc_b" + std::to_string(band) + "_mode"; }
inline std::string getExciterBandDriveID(size_t band)  { return "exc_b" + std::to_string(band) + "_drive"; }
inline std::string getExciterBandMixID(size_t band)    { return "exc_b" + std::to_string(band) + "_mix"; }

// Transient & Sustain Processor Global & Band Helpers
inline constexpr std::string_view TransientBypass     = "transient_bypass";
inline std::string getTransientBandEnableID(size_t band)  { return "trans_b" + std::to_string(band) + "_enable"; }
inline std::string getTransientBandAttackID(size_t band)  { return "trans_b" + std::to_string(band) + "_attack"; }
inline std::string getTransientBandSustainID(size_t band) { return "trans_b" + std::to_string(band) + "_sustain"; }

// Pre-Limiter Soft Clipper
inline constexpr std::string_view ClipperEnable    = "clipper_enable";
inline constexpr std::string_view ClipperCeiling   = "clipper_ceiling";
inline constexpr std::string_view ClipperKnee      = "clipper_knee";

// Limiter & Maximizer
inline constexpr std::string_view LimiterBypass   = "limiter_bypass";
inline constexpr std::string_view LimiterCeiling  = "limiter_ceiling";
inline constexpr std::string_view LimiterThreshold= "limiter_threshold";
inline constexpr std::string_view LimiterRelease  = "limiter_release";
inline constexpr std::string_view LimiterLookahead= "limiter_lookahead";

} // namespace strata::param
