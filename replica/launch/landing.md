# StrataMaster — Landing Page Specification
Generated via `replica-launch`

## Hero Section
- **Headline**: Precision Audio Mastering. Instant Load. Under 3% CPU.
- **Subheadline**: An advanced-class mastering suite built with modern C++20. Pristine dynamic EQ, 4-band phase-compensated crossover, polyphase true-peak limiting, and broadcast-grade LUFS analysis—without bloated installers or background daemons.
- **Primary CTA**: Download StrataMaster (Free Trial / Buy Perpetual)
- **Secondary CTA**: Listen to Audio Demos (Uncompressed WAV A/B)

## The Problem
- **Bloatware in the Studio**: Modern mastering suites require multi-gigabyte downloaders, invasive background authorization daemons, and internet connections that fail when you need them most.
- **CPU Choke**: Trying to master on the mix bus or running multiband oversampling stalls your DAW's audio thread with latency and buffer spikes.
- **Black-Box Presets**: Generic "AI assistants" hide their processing decisions, making unpredictable phase alterations.

## How It Works
1. **Insert on Master Bus**: Drop `StrataMaster.vst3` onto your stereo output in Steinberg Cubase, Ableton Live, Logic, or FL Studio. Loads in under 150 ms.
2. **Listen & Target**: Click *Learn Master* to analyze your track's spectral tilt, crest factor, and loudness against curated target profiles (Streaming, Club EDM, Pop, Acoustic).
3. **Sculpt with Transparency**: Tweak 8-band dynamic EQ curves directly on the real-time 60 FPS spectrum display, sculpt 4-band stereo width with 100% mono cancellation guarantee, and hit exact LUFS targets with zero true-peak inter-sample overs.

## Features Built for Engineers
- **Parametric & Dynamic EQ**: 8-band Direct Form II Transposed biquad filters with envelope-driven modulation.
- **4-Band Multiband Dynamics**: Linkwitz–Riley 4th-order (24 dB/oct) crossover flat to 0.003 dB with soft-knee downward compression and auto-makeup.
- **Harmonic Exciter**: Polyphase 4x/8x oversampled Tape, Tube, Warm, and Retro saturation with $>92\text{ dBFS}$ stopband rejection.
- **Microdynamics & Transient Shaper**: Multiband attack punch and sustain shaping with steady-state ripple squelch.
- **Haas Stereoizer & Mono-Maker**: Sub-bass mono anchor with decorrelation injected solely into the Side channel for 100% comb-free mono translation.
- **Lookahead True-Peak Limiter**: 3.0 ms lookahead, 4x polyphase inter-sample peak detector (ITU-R BS.1770-4), and quadratic soft clipper.
- **Precision Analysis**: EBU R128 Momentary, Short-Term, and Dual-Gated Integrated LUFS, Loudness Range (LRA), and 4096-pt FFT RTA.
- **Reference Track Player**: Gain-matched A/B comparison with 20 ms click-free crossfading.

## FAQ
- **Is an internet connection or iLok required?** No. StrataMaster is 100% self-contained. No background daemons, no PACE services, no telemetry.
- **What plugin formats are supported?** 64-bit VST3 and a standalone desktop application for Windows 10/11 (and macOS universal binary).
- **Can I run it on older machines or laptops?** Yes. StrataMaster's optimized C++20 DSP runs the entire 10-module chain at under 3% CPU usage on modern multi-core processors.
