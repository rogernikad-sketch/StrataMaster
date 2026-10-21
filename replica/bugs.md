# StrataMaster — QA Bug Log & Triage
Generated via `replica-test`

---

### BUG-001: Repetitive Ticking and Audio Silence on Host Latency Updates and Zero-Initialized Smoothed Values

- **Severity**: S1 (Audio Dropouts & Audible Clicks)
- **Flow / Case**: F01, F06, F01-E1
- **Screen**: S01 (Master Shell), S06 (Maximizer)
- **Build**: Pre-v1.0.0 (`31a4093`) | Environment: Standalone & VST3 Host (Cubase 14)

#### Steps to Reproduce
1. Load StrataMaster plugin into DAW audio master insert.
2. Feed continuous sine or pink noise audio.
3. Observe intermittent audio silence or repetitive 1-sample clicks when oversampler or lookahead mode was queried.
4. Toggle global bypass or change plugin parameters while streaming.

#### Expected Behavior
Continuous, pristine audio throughput without clicks, ticks, buffer drops, or silence.

#### Actual Behavior
1. Audio stream muted for several buffers due to `SmoothedValue` instances initializing to 0.0 with slow ramp times.
2. Intersample true-peak limiter lookahead reallocated latency on every audio block instead of checking for parameter modifications, causing host latency ping-pong and buffer resets.

#### Suspected Cause
1. `prepareToPlay` did not prime smoother target values with current APVTS values, causing a fade-in from 0.0 to 1.0.
2. `setLatencySamples` called unconditionally in `processBlock` whenever lookahead was enabled, triggering the host engine to flush audio buffers repeatedly.

#### Resolution & Evidence
- Prime all gain and filter smoothers directly in `prepareToPlay` using `setCurrentAndTargetValue()`.
- Added atomic dirty-flag check for latency: `setLatencySamples` is only invoked when `lookahead` or `oversampling` parameter values actually change.
- Automated Verification Tests:
  - `test_audio_stream.cpp`: Test 36 (`StrataAudioProcessor continuous audio stream and click-free processing` - PASSED)
  - `test_audio_stream.cpp`: Test 37 (`StrataAudioProcessor latency updates only on actual parameter modification` - PASSED)
  - `test_audio_stream.cpp`: Test 38 (`StrataAudioProcessor bypass toggle maintains continuous audio output` - PASSED)
- **Status**: FIXED in commit `31a4093` (Zero open S1 bugs).
