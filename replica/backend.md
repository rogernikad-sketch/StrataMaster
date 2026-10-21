# StrataMaster Audio Backend & DSP Verification
Generated via `replica-backend`

## 1. APVTS Parameter Wiring Checklist

- [x] **Global Parameters**: `input_gain`, `output_gain`, `bypass`, `stereo_mode`, `stereo_width`
- [x] **Stereo Imager**: `stereo_monomaker_enable`, `stereo_monomaker_freq`, `stereo_stereoizer_enable`, `stereo_stereoizer_amount`
- [x] **Equalizer**: `eq_bypass`, `eq_b[0..7]_enable`, `eq_b[0..7]_type`, `eq_b[0..7]_freq`, `eq_b[0..7]_gain`, `eq_b[0..7]_q`, `eq_b[0..7]_dyn_enable`, `eq_b[0..7]_dyn_thresh`, `eq_b[0..7]_dyn_range`
- [x] **Multiband Dynamics**: `dynamics_bypass`, `crossover_split1`, `crossover_split2`, `crossover_split3`, `dyn_b[0..3]_enable`, `dyn_b[0..3]_bypass`, `dyn_b[0..3]_solo`, `dyn_b[0..3]_mute`, `dyn_b[0..3]_thresh`, `dyn_b[0..3]_ratio`, `dyn_b[0..3]_attack`, `dyn_b[0..3]_release`, `dyn_b[0..3]_makeup`, `dyn_b[0..3]_automakeup`
- [x] **Harmonic Exciter**: `exciter_bypass`, `exciter_oversampling`, `exc_b[0..3]_enable`, `exc_b[0..3]_mode`, `exc_b[0..3]_drive`, `exc_b[0..3]_mix`
- [x] **Transient Processor**: `transient_bypass`, `trans_b[0..3]_enable`, `trans_b[0..3]_attack`, `trans_b[0..3]_sustain`
- [x] **Pre-Limiter Soft Clipper**: `clipper_enable`, `clipper_ceiling`, `clipper_knee`
- [x] **Maximizer True-Peak Limiter**: `limiter_bypass`, `limiter_ceiling`, `limiter_threshold`, `limiter_release`, `limiter_lookahead`

---

## 2. Real-Time Thread Safety & Audio Invariants

- [x] **No Heap Allocations in Audio Thread**: All delay buffers, FIR filter kernels, oversampling polyphase branches, and FFT scratch buffers preallocated in `prepareToPlay()`.
- [x] **Lock-Free Communication**: Audio thread communicates telemetry to GUI via single-producer single-consumer ring buffers (`RingBuffer.h`) with zero locks or blocking primitives.
- [x] **Preserved Latency Gating**: `setLatencySamples()` only invoked when oversampling factor or lookahead mode changes, eliminating DAW buffer reallocations, repetitive ticking, and silence dropouts.
- [x] **Denormal Protection**: RAII `ScopedNoDenormals` active on entry of every audio callback.
- [x] **Continuous Smoothers**: Parameter values smoothed using exponential and linear smoothers, preserving state across audio buffers.
