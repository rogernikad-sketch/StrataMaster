# StrataMaster Architecture & Engineering Specification
Generated via `replica-architect`

## 1. System Stack & Tooling

| Layer | Technology Choice | Architectural Justification |
|---|---|---|
| **Audio Framework** | JUCE 7.0.12 (C++20) | Industry-standard audio plugin framework providing native VST3 and Standalone targets with rock-solid host integration. |
| **DSP Core** | Modern C++20 Header-Only / Modular Math | Zero GUI dependencies, pure mathematical DSP, fully SIMD-vectorizable, zero allocations in the real-time audio thread. |
| **State & Parameters**| `juce::AudioProcessorValueTreeState` (APVTS) | Atomic thread-safe parameter synchronization between DAW host automation, GUI, and audio rendering engine. |
| **Thread Communication**| Lock-free SPSC Ring Buffer (`RingBuffer.h`) | Non-blocking telemetry transfer from the high-priority real-time audio thread to the 60 FPS GUI thread. |
| **Vector GUI** | JUCE Graphics & Custom LookAndFeel | Hardware-accelerated 2D vector rendering with dark theme, responsive resizing, interactive nodes, and smooth 60 FPS ballistics. |
| **Testing & CI** | Catch2 v3 + CTest + GitHub Actions | Automated regression testing covering biquad curves, crossover all-pass reconstruction, THD, oversampling, and real-time CPU budget. |
| **Build System** | CMake 3.22+ & MSVC / Clang | Clean multi-platform build setup generating native `.vst3` bundles and standalone executables. |

---

## 2. Signal Processing Graph & Routing Architecture

```
[Master Input Left/Right]
           │
           ▼
   [Input Gain Fader] (±18 dB smoothed)
           │
           ▼
[Mid/Side Matrix Encoder] (Bit-exact √2 energy conservation)
           │
           ▼
[Equalizer Module (S02)]
   ├── 8-Band Parametric Filterbank (Bell, High-Pass, Low-Pass, Shelf)
   └── Envelope-Modulated Dynamic EQ per band
           │
           ▼
[4-Band Linkwitz-Riley Crossover (LR4)] (Phase-compensated 24 dB/oct splits)
   │
   ├─► [Band 1: Low (20 Hz - Split 1)]
   │     ├─► Multiband Dynamics (Downward compressor / upward expander)
   │     ├─► Harmonic Exciter (Tape/Tube/Warm/Retro waveshaping)
   │     └─► Microdynamic Transient Shaper (Attack/Sustain differentiation)
   │
   ├─► [Band 2: Low-Mid (Split 1 - Split 2)]
   │     ├─► Multiband Dynamics
   │     ├─► Harmonic Exciter
   │     └─► Microdynamic Transient Shaper
   │
   ├─► [Band 3: High-Mid (Split 2 - Split 3)]
   │     ├─► Multiband Dynamics
   │     ├─► Harmonic Exciter
   │     └─► Microdynamic Transient Shaper
   │
   └─► [Band 4: High (Split 3 - 20 kHz)]
         ├─► Multiband Dynamics
         ├─► Harmonic Exciter
         └─► Microdynamic Transient Shaper
           │
           ▼
[Crossover Phase-Compensated Summation] (0.003 dB flat magnitude reconstruction)
           │
           ▼
[Stereo Imager Module (S05)]
   ├── 4-Band Stereo Width Modulator
   ├── Mono-Maker Sub-Bass High-Pass Filter on Side Channel (20 - 400 Hz)
   └── Side-Injected Haas Stereoizer (Mono-null guaranteed)
           │
           ▼
[Mid/Side Matrix Decoder]
           │
           ▼
[Pre-Limiter Soft Clipper] (Hyperbolic tangent soft-knee saturation)
           │
           ▼
[Maximizer True-Peak Limiter (S06)]
   ├── 4x Polyphase Oversampling BS.1770 Inter-sample Peak Detector
   ├── 3.0 ms Lookahead Delay Buffer
   └── Dual-Ballistic Adaptive Gain Computer
           │
           ▼
   [Output Gain Fader] (±18 dB smoothed)
           │
           ▼
[A/B Reference Crossfade Switcher]
           │
           ▼
[Broadcast Analysis Bus] (BS.1770-4 LUFS, True-Peak, 4096-pt FFT RTA, Correlation)
           │
           ▼
[Master Output Left/Right]
```

---

## 3. Modular GUI Architecture

The user interface is structured into four distinct physical regions:

```
┌────────────────────────────────────────────────────────────────────────┐
│  HEADER BAR: StrataMaster Logo | Preset Menu | Assistant | A/B | Bypass │
├────────────────────────────────────────────────────────────────────────┤
│  MODULE RACK STRIP: [EQ] [Dynamics] [Exciter] [Imager] [Maximizer]     │
├─────────────────────────────────────────────────┬──────────────────────┤
│                                                 │ MASTER I/O & METERS: │
│                                                 │                      │
│            ACTIVE MODULE WORKSPACE              │ [In Gain & Meter]    │
│                                                 │                      │
│  (Switches dynamically based on selected module:│ [LUFS M / S / I]     │
│   - S02 Equalizer Node Graph + Band Table       │ [True Peak dBTP]     │
│   - S03 Multiband Dynamics + 4-Band Crossover   │ [Correlation Bar]    │
│   - S04 Harmonic Exciter + Mode & Drive         │                      │
│   - S05 Stereo Imager + Mono-Maker + Width      │ [Out Gain & Meter]   │
│   - S06 Maximizer + Ceiling + Threshold + Clip  │                      │
│   - S07 Master Assistant Guided Analysis)       │                      │
│                                                 │                      │
└─────────────────────────────────────────────────┴──────────────────────┘
```

---

## 4. Real-Time Thread Safety & Invariant Guarantees

1. **Zero Allocations**: Zero calls to `malloc`, `new`, `std::vector::push_back`, or string formatting inside `processBlock()`. All memory for oversampling, FFT, lookahead, and delay lines is preallocated in `prepareToPlay()`.
2. **Lock-Free Communication**: UI meters and spectrum analyzers pull data through single-producer single-consumer circular buffers (`RingBuffer`). Audio thread writes lock-free with non-blocking drops if full.
3. **No Latency Ping-Ponging**: Host latency reporting (`setLatencySamples()`) is strictly gated so it is called only when oversampling factors or lookahead modes change, preventing DAW buffer resets, clicks, and dropouts.
4. **Denormal Suppression**: `juce::ScopedNoDenormals` active in all processing callbacks.
5. **Continuous Smoothers**: Parameter smoothing instances maintain state across processing blocks, guaranteeing zero audio clicks or pops.

---

## 5. Build Order & Implementation Milestones

1. **Architecture & Recon Verification**: Validate recon specs and update features matrix.
2. **Design Tokens & Component Specs**: Generate `tokens.json`, `tokens.css`, and `components.md`.
3. **Module Views Implementation**:
   - `ModuleRackBar`: Top module strip with tabs for EQ, Dynamics, Exciter, Imager, Maximizer, power bypass toggles, and active state highlights.
   - `EqualizerView`: 8-band interactive node graph + band table with dynamic EQ controls.
   - `DynamicsView`: 4-band crossover display + band controls (Threshold, Ratio, Attack, Release, Makeup, Solo/Mute, Gain reduction meter).
   - `ExciterView`: 4-band saturation controls (Tape, Tube, Warm, Retro), Drive, Mix, Oversampling.
   - `ImagerView`: 4-band width sliders, Mono-Maker cutoff slider, Haas Stereoizer amount.
   - `MaximizerView`: Threshold slider, Ceiling slider, Soft Clipper knee/ceiling, Release slider, Gain reduction history.
   - `MasterAssistantView`: Target profile selector, 5-second listening visualizer, target vs measured curve, Apply Master button.
4. **StrataAudioEditor Shell Integration**: Connect module views to `ModuleRackBar` navigation, embed Master I/O panel and BS.1770 meters.
5. **Backend Wiring & Verification**: Verify all controls bind to APVTS parameters and control DSP in real-time.
6. **Automated Testing & DAW Verification**: Compile Standalone and VST3, run all Catch2 tests, verify audio continuity.
