# StrataMaster Test Plan & Regression Report
Generated via `replica-test`

## Automated Test Suite: 39 / 39 Tests Passing (100% Green)

| Test ID | Area | Focus | Verification Case | Status | Time |
|---|---|---|---|---|---|
| **T01** | dsp/common | DecibelUtils conversion accuracy | Linear to dB and dB to linear conversions | PASSED | 0.01s |
| **T02** | dsp/common | SmoothedValue linear smoothing | Glitch-free linear interpolation across blocks | PASSED | 0.01s |
| **T03** | dsp/common | RingBuffer SPSC push/pop | Lock-free lockless multi-thread safety | PASSED | 0.01s |
| **T04** | dsp/common | DenormalGuard RAII | Hardware FTZ/DAZ denormal protection | PASSED | 0.01s |
| **T05** | dsp/stereo | Mid/Side bit-exact null test | Energy conservation √2 reconstruction | PASSED | 0.01s |
| **T06** | dsp/stereo | Mono collapse test | Complete cancellation with width = 0.0 | PASSED | 0.01s |
| **T07** | dsp/stereo | Mono-Maker sub-bass attenuation | 24 dB/oct Side channel roll-off below cutoff | PASSED | 0.01s |
| **T08** | dsp/stereo | Stereoizer mono compatibility | 100% cancellation in mono, zero comb-filtering | PASSED | 0.01s |
| **T09** | dsp/filter | Biquad theoretical response | Bell, shelf, HPF, LPF response accuracy | PASSED | 0.01s |
| **T10** | dsp/filter | Biquad impulse response & stability | Filter stability under extreme Q and Nyquist | PASSED | 0.01s |
| **T11** | dsp/dyn_eq | Dynamic EQ selective trigger | Level-dependent gain modulation | PASSED | 0.01s |
| **T12** | dsp/crossover | Linkwitz-Riley flat summation | 4-band all-pass summation flat to 0.003 dB | PASSED | 0.02s |
| **T13** | dsp/crossover | Steep band isolation | Crossover band separation >48 dB/oct | PASSED | 0.01s |
| **T14** | dsp/dynamics | SingleBandCompressor transfer curves | Soft-knee compression ratio and threshold | PASSED | 0.01s |
| **T15** | dsp/dynamics | MultibandDynamics 4-band stereo | Independent band compression, solo & mute | PASSED | 0.01s |
| **T16** | dsp/oversampler | Oversampler latency & flatness | Passband ripple <0.01 dB, latency reporting | PASSED | 0.04s |
| **T17** | dsp/oversampler | Anti-aliasing foldover suppression | Stopband rejection >92 dB | PASSED | 0.02s |
| **T18** | dsp/saturator | Saturator harmonic profile | Tape, Tube, Warm, Retro harmonic analysis | PASSED | 0.06s |
| **T19** | dsp/saturator | Mix and delay compensation | Time-aligned dry/wet blend | PASSED | 0.02s |
| **T20** | dsp/transient | Microdynamic envelope differentiation | Attack and sustain transient separation | PASSED | 0.02s |
| **T21** | dsp/limiter | SoftClipper transfer curve | Hyperbolic tangent saturation ceiling | PASSED | 0.01s |
| **T22** | dsp/limiter | TruePeakLimiter brickwall ceiling | Zero overshoot with 3.0 ms lookahead | PASSED | 0.02s |
| **T23** | analysis | BS.1770 K-Weighting Filter | Stage 1 shelving & Stage 2 high-pass accuracy | PASSED | 0.01s |
| **T24** | analysis | 1 kHz Reference Sine Calibration | Exactly -18.00 LUFS calibration | PASSED | 0.02s |
| **T25** | analysis | Dual-Threshold Gating Behavior | Absolute (-70 LUFS) & relative (-10 LU) gating | PASSED | 0.04s |
| **T26** | analysis | True-Peak Inter-Sample Peak | 4x polyphase inter-sample detection | PASSED | 0.01s |
| **T27** | analysis | 4096-pt Radix-2 FFT Analyzer | Spectral bin energy and frequency resolution | PASSED | 0.01s |
| **T28** | analysis | Feature Extractor Crest Factor | Accurate dynamic range and peak-to-RMS ratio | PASSED | 0.01s |
| **T29** | engine | PresetManager Factory Presets | State serialization, XML persistence | PASSED | 0.02s |
| **T30** | engine | ReferencePlayer A/B Crossfade | 20 ms click-free crossfade & gain match | PASSED | 0.02s |
| **T31** | assistant | MasteringProfiles Integrity | Targets for Streaming, CD, Club, Classical | PASSED | 0.01s |
| **T32** | assistant | AutoMasterEngine Calculations | Spectral Match EQ & Limiter Threshold | PASSED | 0.02s |
| **T33** | assistant | ReferenceMatcher Match Curves | 8-band match curve derivation | PASSED | 0.02s |
| **T34** | performance | Full Mastering DSP Chain Benchmark | CPU budget <2.8%, >35x faster than real-time | PASSED | 0.27s |
| **T35** | performance | 96 kHz Studio Processing | High-resolution 96 kHz performance budget | PASSED | 0.32s |
| **T36** | engine/host | Continuous audio stream & click-free | No dropouts or repetitive ticking | PASSED | 0.04s |
| **T37** | engine/host | Latency update stability | Latency called on modification only | PASSED | 0.02s |
| **T38** | engine/host | Bypass toggle continuity | Continuous audio throughput on bypass | PASSED | 0.02s |
| **T39** | gui | Modular GUI Instantiation & Resizing | 6 module views, 960x580 to 1440x900 scaling | PASSED | 0.08s |
