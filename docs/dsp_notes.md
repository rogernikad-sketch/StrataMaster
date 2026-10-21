# StrataMaster DSP Mathematical Specifications

## 1. Mid/Side Matrix
The Mid/Side encoder transforms standard stereo Left ($L$) and Right ($R$) channels into Center/Mid ($M$) and Stereo/Side ($S$) components:
$$M = \frac{L + R}{\sqrt{2}}, \quad S = \frac{L - R}{\sqrt{2}}$$
The inverse transformation (decoder) restores Left and Right with complete bit-exact conservation:
$$L = \frac{M + S}{\sqrt{2}}, \quad R = \frac{M - S}{\sqrt{2}}$$
Normalization by $\frac{1}{\sqrt{2}} \approx 0.70710678$ ensures 3 dB center conservation so overall acoustic power is preserved across encoding and decoding.

## 2. Linkwitz-Riley 4th-Order (LR4) Crossover
For multiband splitting, LR4 filters are formed by cascading two identical 2nd-order Butterworth filters in series:
- Low-Pass: $H_{\text{LP}}(s) = \left( \frac{\omega_c^2}{s^2 + \sqrt{2}\omega_c s + \omega_c^2} \right)^2$
- High-Pass: $H_{\text{HP}}(s) = \left( \frac{s^2}{s^2 + \sqrt{2}\omega_c s + \omega_c^2} \right)^2$

Properties:
- Phase difference between Low-Pass and High-Pass outputs is exactly $360^\circ$ ($0^\circ$).
- Magnitude sum at the crossover frequency $f_c$ is precisely $-6\text{ dB} + (-6\text{ dB}) = 0\text{ dB}$ (flat magnitude response).
- Recombination yields an all-pass transfer function with zero magnitude ripple ($\pm 0.0\text{ dB}$).

## 3. Harmonic Exciter Transfer Functions
To generate analog warmth without digital harshness, signals are 4x oversampled and processed via dedicated waveshapers:
- **Tape Saturation (Odd Harmonics)**:
  $$y = \tanh(k \cdot x)$$
- **Tube Triode Saturation (Asymmetric 2nd & 3rd Harmonics)**:
  $$y = \begin{cases} \frac{x}{1 + |x|} & x \ge 0 \\ \frac{x}{1 + 0.5|x|} & x < 0 \end{cases}$$
- **Warm Saturation (Even Harmonics)**:
  $$y = x + \alpha(x^2 - |x|)$$

## 4. True-Peak Limiter (ITU-R BS.1770-4)
- **Oversampling**: 4x polyphase FIR interpolation oversampling reconstructs inter-sample analog peaks.
- **Lookahead**: A 3.0 ms ring-buffer delay allows instantaneous peak detection before the audio reaches the gain reduction multiplier.
- **Ballistics**: Instantaneous attack with adaptive dual-release (fast transient decay $\tau \approx 20\text{ ms}$, slow program RMS decay $\tau \approx 250\text{ ms}$).
