# Recon Map: iZotope Ozone 11 Advanced (Desktop VST3 / Standalone)

Scope: Complete mastering suite (Master Shell, Module Chain Rack, Equalizer, Multiband Dynamics, Harmonic Exciter, Stereo Imager, Maximizer / True-Peak Limiter, Master Assistant, Reference Player, BS.1770 Broadcast Metering).  
For: Professional mastering engineers, mixing producers, and self-mastering creators who demand pristine acoustic precision, broadcast compliance, and intelligent assistance without intrusive DRM or CPU bloat.  
Date: 2026-10-21  

---

## Sources

| # | Source | URL / Reference | Notes |
|---|---|---|---|
| 1 | iZotope Ozone 11 Help Center & Documentation | `izotope.com/en/products/ozone.html` | Full parameter ranges, filter curves, module specs |
| 2 | Sound On Sound Ozone 11 In-Depth Review | `soundonsound.com/reviews/izotope-ozone-11` | Real-world mastering workflows, UI ergonomics, feature analysis |
| 3 | ITU-R BS.1770-4 / EBU R128 Recommendation | `itu.int/rec/R-REC-BS.1770` | Precise gating, K-weighting filter specifications, True-Peak detection |
| 4 | Clean-room StrataMaster Repository | `C:\dev\StrataMaster` | Existing C++20 / JUCE 7 architecture, 38/38 passing unit tests |

---

## Core Loop

One continuous mastering chain: Load audio or insert into DAW master bus -> analyze acoustic profile via Master Assistant -> shape tone and dynamic balance with dedicated modular processors (EQ, Dynamics, Exciter, Imager, Maximizer) -> verify broadcast loudness compliance (LUFS/TP) and reference parity -> export master.

---

## Screens & Views Inventory

| ID | Screen / View | Route / Access | Purpose | Key Components | States Seen |
|---|---|---|---|---|---|
| **S01** | Master Shell & Module Rack | Default top-level frame | Top-level host frame, preset management, global routing, master I/O | Top bar (Presets, Master Assistant button, Global Bypass, Gain Match, A/B Audition), Module Chain Strip (EQ, Dynamics, Exciter, Imager, Maximizer), Master I/O faders, BS.1770 LUFS meter, Correlation meter | Active, Bypassed, Gain-Matched, A/B Toggled |
| **S02** | Equalizer Module View | Click `EQ` in Module Rack | 8-band parametric & dynamic frequency shaping | Interactive 8-band draggable node graph, 4096-pt RTA spectrum overlay, Band control table (Band 1..8, Filter Type, Freq, Gain, Q, Dynamic toggle, Dynamic Range/Threshold), Stereo / Mid-Side processing selector | Stereo, Mid/Side, Dynamic Active, Band Solo |
| **S03** | Multiband Dynamics View | Click `Dynamics` in Module Rack | 4-band independent compression & expansion | 4-band crossover graph with draggable frequency split lines (F1, F2, F3), Band selection tabs (Band 1..4), Threshold, Ratio, Attack, Release, Makeup Gain, Auto-Makeup toggle, Solo/Mute buttons, Real-time gain reduction meters | 4 Bands active, Individual Band Soloed/Muted, Auto-Makeup Active |
| **S04** | Harmonic Exciter View | Click `Exciter` in Module Rack | Multiband analog saturation and acoustic warmth | 4-band crossover graph, Saturation mode selector (Tape, Tube, Warm, Retro), Drive knob (0-100%), Mix blend knob (0-100%), Oversampling selector (None, 2x, 4x, 8x) | Clean, Saturated, Oversampled (2x/4x/8x), Band Solo |
| **S05** | Stereo Imager View | Click `Imager` in Module Rack | Multiband stereo width sculpting and mono bass protection | 4-band crossover width sliders (-100% to +100%), Haas Stereoizer toggle and delay/depth slider, Mono-Maker sub-bass anchor HPF cutoff slider (20 - 400 Hz), Phase correlation readout (-1.0 to +1.0) | Full Stereo, Monofied Bass, Haas Decorrelated, Out-of-phase Warning |
| **S06** | Maximizer / True-Peak Limiter View | Click `Maximizer` in Module Rack | BS.1770 True-Peak brickwall limiting and loudness maximization | Limiter Ceiling slider (-12 to 0 dBTP), Threshold slider (-24 to 0 dB), Release time slider (10 to 1000 ms), Lookahead mode toggle (3.0 ms), Pre-limiter Soft Clipper toggle & knee slider, Scrolling gain reduction display | Idling, Limiting, Soft Clipping Active, True-Peak Warning |
| **S07** | Master Assistant View | Click `Master Assistant` button | AI-driven acoustic analysis and automated master chain configuration | Target profile selector (Streaming -14 LUFS, CD/Club -9 LUFS, High Dynamics -18 LUFS, Reference Match), 5-second real-time listening progress ring, Spectral centroid and crest factor target delta comparison, Match EQ curve generator, "Apply Master" button, "Re-Analyze" button | Idle, Listening (0-100%), Profile Evaluated, Master Applied |
| **S08** | Reference Player & A/B Panel | Click `Track A/B` or `Gain Match` in Top Bar | Level-matched auditioning against commercial reference tracks | Track A / Track B audition toggle, Loudness gain match compensation button, Reference file loader button, Real-time loudness delta readout | Track A (Plugin Master), Track B (Reference Audio), Gain Matched |

---

## User Flows

```
F01 Mastering with Intelligent Assistant (Core Loop)
    S01 Master Shell -> S07 Master Assistant -> Click "Start Listening" -> Play audio in DAW (5 sec) -> 
    View target profile vs measured metrics -> Click "Apply Master" -> Settings applied to EQ, Dynamics, Imager, Maximizer ->
    S01 Verify output loudness and spectrum
    Happy path clicks: 3
    Edge cases: Audio below silence threshold (-60 dBFS), audio clipping during analysis, user switches profile mid-analysis.

F02 Manual Surgical Tone Sculpting (Equalizer)
    S01 Master Shell -> S02 Click "EQ" in Module Rack -> Drag node 3 (2.5 kHz) down 2 dB -> 
    Enable Dynamic EQ for Band 3 -> Set Dynamic Range -3 dB, Threshold -18 dB ->
    Switch to Mid/Side mode -> Boost Band 6 (10 kHz) on Side channel by +1.5 dB ->
    Audition with EQ Bypass toggle
    Happy path clicks: 5
    Edge cases: Extreme Q values, overlapping nodes, phase cancellation in Mid/Side.

F03 Multiband Dynamic Control
    S01 Master Shell -> S03 Click "Dynamics" in Module Rack -> Drag crossover split 1 to 120 Hz ->
    Select Band 1 (Low) -> Set Threshold to -16 dB, Ratio to 3.0:1 -> Enable Auto-Makeup ->
    Solo Band 1 to check low-end thump -> Unsolo -> Return to Master Shell
    Happy path clicks: 6
    Edge cases: Crossover lines crossing each other, infinite ratio, zero attack time.

F04 Saturation & Excitement
    S01 Master Shell -> S04 Click "Exciter" in Module Rack -> Select Band 4 (Highs) ->
    Set Mode to "Tape" -> Increase Drive to 25% -> Set Mix to 40% -> Select 4x Oversampling ->
    Audition harmonic sheen
    Happy path clicks: 5
    Edge cases: Aliasing foldover on low sample rates (mitigated by 4x/8x polyphase filters).

F05 Stereo Width Sculpting & Bass Anchoring
    S01 Master Shell -> S05 Click "Imager" in Module Rack -> Enable Mono-Maker ->
    Set Mono-Maker frequency to 120 Hz -> Set Band 4 (Highs) width to +35% ->
    Enable Stereoizer on Mid-Highs -> Observe Correlation Meter stays above +0.4 ->
    Test Mono Compatibility button
    Happy path clicks: 5
    Edge cases: Negative phase correlation, extreme Haas delays causing comb filtering.

F06 Final True-Peak Brickwall Maximization
    S01 Master Shell -> S06 Click "Maximizer" in Module Rack -> Set Ceiling to -1.0 dBTP ->
    Pull Threshold down to -8.0 dB -> Enable Soft Clipper at -0.5 dB ->
    Observe Gain Reduction meter and output BS.1770 Integrated LUFS hitting -14.0 LUFS
    Happy path clicks: 4
    Edge cases: Intersample peaks exceeding 0 dBTP without oversampling (mitigated by 4x BS.1770 polyphase detector).
```

---

## Component Architecture

| Component | Variants | States | Used In |
|---|---|---|---|
| `ModuleTabButton` | Rack Tab (EQ, Dyn, Exc, Img, Max) | Normal, Active, Hover, Bypassed | S01, S02, S03, S04, S05, S06 |
| `BypassButton` | Power Icon / Toggle | Active (On), Bypassed (Off), Hover | All Modules |
| `Knob` | Small (60px), Medium (80px), Large (100px) | Default, Dragging, Double-click reset, Value text | All Modules |
| `FaderSlider` | Vertical Input/Output/Limiter | Default, Dragging, Hover | S01, S06 |
| `EQNodeGraph` | 8 draggable nodes + interactive response curve | Idle, Hover Node, Dragging Frequency/Gain, Right-click Type | S02, S07 |
| `CrossoverGraph` | 4 bands + 3 draggable split handles | Normal, Handle Dragging, Band Hover | S03, S04, S05 |
| `LevelMeter` | Stereo Peak Bar (-60 to +6 dB) with clip LED | Real-time audio response, Peak Hold, Clip Warning | S01 |
| `LoudnessMeterBar` | BS.1770 Momentary, Short-Term, Integrated, LRA | Real-time broadcast response, Target marker | S01, S06, S07 |
| `CorrelationMeter` | -1.0 to +1.0 Phase Correlation Bar | Real-time phase response, In-phase (Cyan), Warning (Red) | S01, S05 |
| `SpectrumDisplay` | 4096-pt Radix-2 FFT RTA with 1/3 octave smoothing | Real-time 60 FPS, Pre/Post comparison | S01, S02, S07 |
| `PresetComboBox` | Modern flat dropdown | Closed, Opened, Selected | S01 |

---

## Inferred Data Model (APVTS Parameters)

```
ParameterTree
  ├── Global (InputGain, OutputGain, Bypass, StereoMode, StereoWidth)
  ├── Equalizer
  │    ├── eq_bypass (bool)
  │    └── eq_b[0..7] (enable, type, freq, gain, q, dyn_enable, dyn_thresh, dyn_range)
  ├── MultibandDynamics
  │    ├── dynamics_bypass (bool)
  │    ├── crossover_split[1..3] (float Hz: 120, 1500, 6000)
  │    └── dyn_b[0..3] (enable, bypass, solo, mute, thresh, ratio, attack, release, makeup, automakeup)
  ├── HarmonicExciter
  │    ├── exciter_bypass (bool)
  │    ├── exciter_oversampling (enum: None, 2x, 4x, 8x)
  │    └── exc_b[0..3] (enable, mode [Warm, Tape, Tube, Retro], drive, mix)
  ├── StereoImager
  │    ├── stereo_monomaker_enable (bool)
  │    ├── stereo_monomaker_freq (float Hz: 20..400)
  │    ├── stereo_stereoizer_enable (bool)
  │    ├── stereo_stereoizer_amount (float: 0..100%)
  │    └── stereo_b[0..3]_width (float: 0..200%)
  ├── Maximizer
  │    ├── limiter_bypass (bool)
  │    ├── limiter_threshold (float dB: -24..0)
  │    ├── limiter_ceiling (float dB: -12..0)
  │    ├── limiter_release (float ms: 10..1000)
  │    ├── limiter_lookahead (bool)
  │    ├── clipper_enable (bool)
  │    ├── clipper_ceiling (float dB: -12..0)
  │    └── clipper_knee (float dB: 0..12)
  └── Assistant
       ├── target_profile (enum: Streaming, CD, Club, Classical)
       ├── match_intensity (float: 0..100%)
       └── listening_state (enum: Idle, Listening, Ready, Applied)
```

---

## Out of Scope (Clean-Room Boundary)

- Proprietary iZotope brand assets, logos, fonts, or copyrighted vector artwork.
- iZotope Product Portal, PACE iLok anti-piracy daemons, or telemetry daemons.
- Closed-source proprietary Neural Network weights or binary model extraction.
- Non-standard closed cloud preset libraries.

---

## Sizing

- **Views / Modules**: 8 views (Shell S01, EQ S02, Dynamics S03, Exciter S04, Imager S05, Maximizer S06, Assistant S07, Reference S08).
- **User Flows**: 6 core mastering flows.
- **Parameters**: 68 thread-safe APVTS parameters.
- **Size**: **M** (Modular mastering suite with production-grade C++20 DSP and hardware-accelerated JUCE GUI).
