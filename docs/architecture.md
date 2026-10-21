# StrataMaster Architecture Guide

## 1. System Overview
StrataMaster is structured into three clean decoupled layers:
1. **Pure DSP Layer (`src/dsp/`)**:
   - Zero JUCE GUI dependencies.
   - Operates on raw float/double sample buffers or `juce::dsp::AudioBlock`.
   - 100% unit-testable in isolation without audio hardware.
2. **Audio Engine Layer (`src/engine/`)**:
   - Manages audio processing blocks, parameter synchronization via `juce::AudioProcessorValueTreeState` (APVTS).
   - Coordinates latency reporting (`setLatencySamples`) for lookahead limiters and linear-phase filters.
   - Manages preset persistence, XML/JSON state save and restore.
3. **Analysis & GUI Layer (`src/analysis/`, `src/assistant/`, `src/gui/`)**:
   - Asynchronous lock-free data sharing via single-producer single-consumer circular buffers (`RingBuffer`).
   - Hardware-accelerated 60 FPS vector GUI rendering with interactive parametric filter handles and RTA spectrum.

## 2. Signal Routing
```
[Stereo In] 
    │
    ▼
[Input Gain Trim] ──► [Stereo / Mid-Side Matrix Encode]
                            │
                            ▼
                    [Parametric & Dynamic EQ (8 Bands)]
                            │
                            ▼
                    [4-Band Linkwitz-Riley Crossover]
                            │
                    ┌───────┴───────┬───────────────┐
                    ▼               ▼               ▼
                 [Low]           [Mid-Low]       [Mid-High]       [High]
                 ├───────Multiband Compressor / Expander ────────┤
                 ├───────Harmonic Exciter (4x Oversampled) ──────┤
                 └───────Microdynamic Transient / Sustain ───────┘
                            │
                            ▼
                    [Crossover Recombination Sum]
                            │
                            ▼
                    [Stereo Width & Bass Mono-Maker]
                            │
                            ▼
                    [Mid-Side Matrix Decode]
                            │
                            ▼
                    [Pre-Limiter Soft Clipper]
                            │
                            ▼
                    [Lookahead True-Peak Brickwall Limiter]
                            │
                            ▼
                    [Output Gain & Dither Stage]
                            │
                            ▼
                    [Master Audio Out]
```

## 3. Real-Time Audio Safety Invariants
- **No heap allocations** in `processBlock()` (`new`, `malloc`, `std::vector::resize`, `std::string`).
- **No blocking synchronization** (no `std::mutex`, condition variables, or locks in audio thread).
- **Denormal Protection**: RAII `ScopedNoDenormals` active on entry of every audio callback.
- **Click-Free Parameter Updates**: Continuous parameter values smoothed using exponential or linear filters (`SmoothedValue`).
