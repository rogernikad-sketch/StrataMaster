# StrataMaster — Professional Audio Mastering Suite

[![Build and Test](https://github.com/stratamaster/strata-master/actions/workflows/build.yml/badge.svg)](.github/workflows/build.yml)
[![Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![JUCE](https://img.shields.io/badge/JUCE-7.0.12-orange.svg)](https://juce.com)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

StrataMaster is an original, professional audio mastering application and native 64-bit VST3 plugin built with modern C++20 and JUCE 7. Designed under clean-room engineering principles, it provides a comprehensive mastering suite including:

* **Pristine Parametric & Dynamic Equalization** (minimum-phase and linear-phase biquad filterbanks)
* **4-Band Multiband Dynamics** (phase-compensated Linkwitz-Riley crossovers, downward compression, upward expansion)
* **Harmonic Exciter** (oversampled Warm, Tape, Tube, and Retro analog-modeled non-linear saturation)
* **Microdynamics / Impact Transient Shaper** (multiband envelope differentiation)
* **Stereo Imaging** (multiband width, Haas decorrelator, mono-maker bass anchor)
* **True-Peak Limiter & Soft Clipper** (ITU-R BS.1770-4 4x/8x oversampled inter-sample peak prevention)
* **Broadcast Loudness & Spectral Analysis** (Integrated, Short-term, Momentary LUFS, Loudness Range, 4096-pt FFT RTA)
* **Reference Track A/B Comparison** (seamless, click-free level-matched auditioning)
* **Automated Mastering Assistant** (spectral centroid, crest factor, and dynamic range target convergence)

---

## Architecture & Directory Layout

* `src/dsp/`: Pure mathematical DSP with zero GUI dependencies, suitable for isolated unit testing.
  * `src/dsp/common/`: Lock-free ring buffer, parameter smoothers, denormal guards, and decibel conversions.
* `src/engine/`: Thread-safe parameter tree (`juce::AudioProcessorValueTreeState`), mastering chain graph, preset manager, and reference audio streamer.
* `src/analysis/`: High-precision ITU-R BS.1770-4 loudness metering, True-Peak detection, FFT spectrum analyzer, and correlation meters.
* `src/assistant/`: Target mastering profiles (Streaming, CD, Club, Classical) and auto-master decision logic.
* `src/gui/`: Modern dark-theme vector GUI with interactive EQ node graph, module rack, and level meters.
* `tests/`: Automated unit tests verifying DSP correctness, filter response, THD, and latency.

---

## Building from Source

### Prerequisites
* **Windows**: Visual Studio 2022 / 2026 with C++ Desktop Development (MSVC), CMake 3.22+
* **JUCE**: JUCE 7.0.12 (placed at `C:/dev/JUCE` or specified via `-DJUCE_DIR=...`)
* **DAW Host**: Steinberg Cubase 14 (or any VST3 compatible host)

### Configure and Build
```powershell
# Generate CMake build solution
cmake -B build -S . -DJUCE_DIR="C:/dev/JUCE"

# Compile Release binaries (VST3 Plugin and Standalone Application)
cmake --build build --config Release --parallel

# Run Unit Tests
ctest --test-dir build -C Release --output-on-failure
```
