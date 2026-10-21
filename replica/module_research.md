# StrataMaster — Comprehensive Module & Control Research Specification
Generated for Step 3 of the Replica Workflow

---

## 1. Global Master Shell & Controls

### 1.1 Input Gain (`input_gain`)
- **What it does**: Trims input audio level before any DSP stage.
- **DSP Parameter**: Multiplied linearly with input samples ($g = 10^{\text{gain}/20}$) with smoothing.
- **Range & Units**: $-18.0\text{ dB}$ to $+18.0\text{ dB}$, default $0.0\text{ dB}$.
- **Audible Effect**: Adjusts raw incoming level driving into the mastering chain.
- **Meters**: Directly shifts input peak meters.

### 1.2 Output Gain (`output_gain`)
- **What it does**: Post-limiter trim fader for final gain stage.
- **DSP Parameter**: Multiplied linearly with processed samples ($g = 10^{\text{gain}/20}$) with smoothing.
- **Range & Units**: $-18.0\text{ dB}$ to $+18.0\text{ dB}$, default $0.0\text{ dB}$.
- **Audible Effect**: Adjusts final listening/master output level.
- **Meters**: Directly shifts output peak meter and BS.1770 loudness meter.

### 1.3 Global Bypass (`bypass`)
- **What it does**: Bypasses all mastering processing, delivering dry input directly to output with continuous smoothers.
- **Range**: On / Off (boolean). Default Off.
- **Audible Effect**: Instantaneous comparison between raw mix and mastered output. Preserves audio streaming without clicks.

### 1.4 Stereo Processing Mode (`stereo_mode`)
- **What it does**: Configures stereo routing architecture.
- **Options**: Stereo (0), Mid/Side (1), Mid Solo (2), Side Solo (3). Default Stereo.
- **Audible Effect**: In Mid/Side mode, enables separate mid and side processing. Mid Solo isolates mono center; Side Solo isolates stereo ambiance.

### 1.5 Reference Player A/B (`ab_mode`) & Gain Match
- **What it does**: Seamless crossfade between master track A and reference track B, with optional loudness matching.
- **Crossfade Time**: 20 ms click-free equal-power crossfade.
- **Gain Match**: Calculates loudness delta ($\Delta L = L_{\text{ref}} - L_{\text{master}}$) and scales output to eliminate loudness bias.

---

## 2. Equalizer Module (`src/dsp/BiquadFilter.h`, `DynamicEqualizer.h`)

### 2.1 Band Enable (`eq_b[0..7]_enable`)
- **What it does**: Activates or disables individual EQ band.
- **Range**: On / Off (Default: all bands enabled).

### 2.2 Filter Type (`eq_b[0..7]_type`)
- **What it does**: Selects RBJ biquad transfer function.
- **Options**: High-Pass (0), Low-Shelf (1), Bell (2), High-Shelf (3), Low-Pass (4).
- **Default Types**: Band 0 = HPF (20 Hz), Band 1 = Low Shelf (100 Hz), Bands 2-5 = Bell (250, 1000, 3500, 8000 Hz), Band 6 = High Shelf (12000 Hz), Band 7 = LPF (20000 Hz).

### 2.3 Frequency (`eq_b[0..7]_freq`)
- **What it does**: Sets filter center or corner frequency.
- **Range**: $20.0\text{ Hz}$ to $20000.0\text{ Hz}$ (logarithmic scale).

### 2.4 Gain (`eq_b[0..7]_gain`)
- **What it does**: Sets boost or cut magnitude for Bell and Shelf filters.
- **Range**: $-18.0\text{ dB}$ to $+18.0\text{ dB}$, default $0.0\text{ dB}$.

### 2.5 Quality Factor / Q (`eq_b[0..7]_q`)
- **What it does**: Controls filter bandwidth.
- **Range**: $0.1$ to $18.0$, default $0.707$ for shelves/pass, $1.414$ for bells.

### 2.6 Dynamic EQ Toggle (`eq_b[0..7]_dyn_enable`)
- **What it does**: Enables level-dependent dynamic gain modulation on the filter band.
- **Range**: On / Off (Default Off).

### 2.7 Dynamic Threshold (`eq_b[0..7]_dyn_thresh`)
- **What it does**: Sidechain detection threshold for dynamic modulation.
- **Range**: $-48.0\text{ dB}$ to $0.0\text{ dB}$, default $-18.0\text{ dB}$.

### 2.8 Dynamic Range (`eq_b[0..7]_dyn_range`)
- **What it does**: Maximum allowable boost or cut applied dynamically when signal crosses threshold.
- **Range**: $-12.0\text{ dB}$ to $+12.0\text{ dB}$, default $-3.0\text{ dB}$.
- **Audible Effect**: Compresses resonant frequency peaks (de-essing, taming harshness) or expands dynamic transient punch.

---

## 3. Multiband Dynamics Module (`src/dsp/MultibandDynamics.h`, `CrossoverFilterbank.h`)

### 3.1 Crossover Splits (`crossover_split[1..3]`)
- **What it does**: 4th-order Linkwitz-Riley (LR4) phase-compensated crossover split frequencies.
- **Ranges**:
  - Split 1 (Low / Low-Mid): $40.0\text{ Hz}$ to $500.0\text{ Hz}$, default $120.0\text{ Hz}$.
  - Split 2 (Low-Mid / High-Mid): $200.0\text{ Hz}$ to $5000.0\text{ Hz}$, default $1500.0\text{ Hz}$.
  - Split 3 (High-Mid / High): $1000.0\text{ Hz}$ to $18000.0\text{ Hz}$, default $6000.0\text{ Hz}$.
- **Acoustic Precision**: Magnitude sum flat to $0.003\text{ dB}$ across recombination.

### 3.2 Band Threshold (`dyn_b[0..3]_thresh`)
- **What it does**: Sets compression knee onset level per band.
- **Range**: $-40.0\text{ dB}$ to $0.0\text{ dB}$, default $-12.0\text{ dB}$.

### 3.3 Band Ratio (`dyn_b[0..3]_ratio`)
- **What it does**: Compression slope above threshold ($1:1$ to $20:1$).
- **Range**: $1.0:1$ to $20.0:1$, default $2.0:1$.

### 3.4 Attack & Release (`dyn_b[0..3]_attack`, `_release`)
- **Ranges**: Attack $1.0\text{ ms}$ to $200.0\text{ ms}$ (default $25\text{ ms}$); Release $10.0\text{ ms}$ to $1000.0\text{ ms}$ (default $150\text{ ms}$).

### 3.5 Makeup Gain & Auto-Makeup (`dyn_b[0..3]_makeup`, `_automakeup`)
- **What it does**: Manual boost ($-12\text{ dB}$ to $+12\text{ dB}$) or automatic gain compensation based on threshold and ratio:
  $$\text{Gain}_{\text{auto}} = -\frac{\text{Thresh}}{2} \cdot \left(1 - \frac{1}{\text{Ratio}}\right)$$

### 3.6 Band Solo & Mute (`dyn_b[0..3]_solo`, `_mute`)
- **What it does**: Isolates single band or silences it for clinical acoustic inspection.

---

## 4. Harmonic Exciter Module (`src/dsp/Saturator.h`, `Oversampler.h`)

### 4.1 Oversampling Factor (`exciter_oversampling`)
- **What it does**: Polyphase halfband FIR oversampling (None, 2x, 4x, 8x).
- **Anti-Aliasing**: $>92\text{ dB}$ stopband attenuation with linear phase response.
- **Latency**: Automatically calculated and reported to host on change only (guaranteeing no clicks).

### 4.2 Saturation Modes (`exc_b[0..3]_mode`)
- **Options**:
  - `Warm` (0): Even-order harmonics ($x + \alpha(x^2 - |x|)$) adding density.
  - `Tape` (1): Odd-order harmonics ($\tanh(k \cdot x)$) providing punch and tape glue.
  - `Tube` (2): Asymmetric triode transfer function adding musical sheen and presence.
  - `Retro` (3): Vintage transistor soft-clip saturation.

### 4.3 Drive & Mix (`exc_b[0..3]_drive`, `_mix`)
- **Drive**: $0.0\%$ to $100.0\%$ (default $15.0\%$). Scales nonlinear excitation.
- **Mix**: $0.0\%$ to $100.0\%$ (default $30.0\%$). Dry/wet blend with phase alignment.

---

## 5. Stereo Imager Module (`src/dsp/StereoProcessor.h`)

### 5.1 Multiband Stereo Width (`stereo_width`)
- **What it does**: Side channel scaling factor.
- **Range**: $0.0\%$ (Complete Mono) to $200.0\%$ (Super-wide), default $100.0\%$.

### 5.2 Mono-Maker Bass Anchor (`stereo_monomaker_enable`, `stereo_monomaker_freq`)
- **What it does**: 2nd-order high-pass Butterworth filter applied strictly to the Side ($S$) channel below cutoff.
- **Range**: $20.0\text{ Hz}$ to $400.0\text{ Hz}$, default $120.0\text{ Hz}$.
- **Audible Effect**: Focuses low frequencies and kick drum strictly into center channel, eliminating phase cancellation in clubs/vinyl.

### 5.3 Haas Stereoizer (`stereo_stereoizer_enable`, `stereo_stereoizer_amount`)
- **What it does**: Decorrelation delay network injected into Side channel with mono-null guarantee ($S_{\text{decorr}} - S_{\text{decorr}} = 0$ in mono).
- **Range**: $0.0\%$ to $100.0\%$, default $30.0\%$.

---

## 6. Maximizer / True-Peak Limiter Module (`src/dsp/TruePeakLimiter.h`, `SoftClipper.h`)

### 6.1 Limiter Ceiling (`limiter_ceiling`)
- **What it does**: Maximum allowable inter-sample peak level in dB True Peak.
- **Range**: $-12.0\text{ dBTP}$ to $0.0\text{ dBTP}$, default $-1.0\text{ dBTP}$ (EBU R128 / Streaming standard).

### 6.2 Limiter Threshold (`limiter_threshold`)
- **What it does**: Input drive gain into brickwall limiting ceiling.
- **Range**: $-24.0\text{ dB}$ to $0.0\text{ dB}$, default $0.0\text{ dB}$.

### 6.3 Release Time (`limiter_release`)
- **What it does**: Envelope recovery time constant.
- **Range**: $10.0\text{ ms}$ to $1000.0\text{ ms}$, default $100.0\text{ ms}$.

### 6.4 Lookahead Mode (`limiter_lookahead`)
- **What it does**: 3.0 ms delay buffer providing zero-overshoot transient detection.
- **Latency**: 132 samples lookahead + oversampling latency reported to DAW host.

### 6.5 Pre-Limiter Soft Clipper (`clipper_enable`, `clipper_ceiling`, `clipper_knee`)
- **What it does**: Hyperbolic tangent waveshaper before the limiter ceiling to shave ultra-fast transients transparently.

---

## 7. Master Assistant (`src/assistant/AutoMasterEngine.h`, `MasteringProfiles.h`)

### 7.1 Target Profiles
1. **Streaming**: Target $-14.0\text{ LUFS}$, $-1.0\text{ dBTP}$, balanced target curve with gentle roll-off above $16\text{ kHz}$.
2. **CD / Club**: Target $-9.0\text{ LUFS}$, $-0.3\text{ dBTP}$, punchy bass, tight dynamics, aggressive limiting.
3. **High Dynamic / Classical**: Target $-18.0\text{ LUFS}$, $-1.5\text{ dBTP}$, pristine transient preservation, transparent limiting.
4. **Reference Match**: Dynamic target derived directly from loaded reference track FFT profile.

### 7.2 Analysis Routine
- Evaluates real-time audio for 5 seconds (RMS, Peak, Crest Factor, Spectral Centroid, LUFS).
- Generates 8-band Match EQ compensation curve.
- Sets multiband dynamics threshold and auto-makeup based on measured crest factor.
- Adjusts True-Peak Limiter threshold to converge exactly on target Integrated LUFS.
