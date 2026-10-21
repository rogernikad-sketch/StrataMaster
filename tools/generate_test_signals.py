#!/usr/bin/env python3
"""
generate_test_signals.py
Generates calibrated test audio signals for StrataMaster automated testing:
- 1 kHz 0 dBFS and -18 dBFS reference sine waves
- Swept sine wave (20 Hz - 20 kHz)
- Dirac delta impulse signal
- Synthetic inter-sample clipping stress test signal
Zero external dependencies (uses standard library: wave, math, struct).
"""

import math
import struct
import wave
import os

SAMPLE_RATE = 48000
TEST_DATA_DIR = os.path.join(os.path.dirname(__file__), "..", "tests", "test_data")

def ensure_dir(path):
    if not os.path.exists(path):
        os.makedirs(path, exist_ok=True)

def write_wav(filename, samples, sample_rate=SAMPLE_RATE, channels=2):
    filepath = os.path.join(TEST_DATA_DIR, filename)
    with wave.open(filepath, 'w') as wf:
        wf.setnchannels(channels)
        wf.setsampwidth(2) # 16-bit PCM
        wf.setframerate(sample_rate)
        frames = bytearray()
        for s in samples:
            # clamp to [-1.0, 1.0]
            val = max(-1.0, min(1.0, s))
            int_val = int(val * 32767.0)
            packed = struct.pack('<h', int_val)
            for _ in range(channels):
                frames.extend(packed)
        wf.writeframes(frames)
    print(f"Generated: {filepath}")

def generate_sine(freq_hz, duration_sec, amplitude_db=0.0):
    amp = 10.0 ** (amplitude_db / 20.0)
    num_samples = int(SAMPLE_RATE * duration_sec)
    return [amp * math.sin(2.0 * math.pi * freq_hz * i / SAMPLE_RATE) for i in range(num_samples)]

def generate_impulse(duration_sec=0.1):
    num_samples = int(SAMPLE_RATE * duration_sec)
    samples = [0.0] * num_samples
    samples[10] = 1.0 # single Dirac delta impulse
    return samples

def generate_sweep(start_hz=20.0, end_hz=20000.0, duration_sec=3.0, amplitude_db=-6.0):
    amp = 10.0 ** (amplitude_db / 20.0)
    num_samples = int(SAMPLE_RATE * duration_sec)
    samples = []
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        # Logarithmic sweep
        freq = start_hz * ((end_hz / start_hz) ** (t / duration_sec))
        phase = 2.0 * math.pi * start_hz * duration_sec / math.log(end_hz / start_hz) * (((end_hz / start_hz) ** (t / duration_sec)) - 1.0)
        samples.append(amp * math.sin(phase))
    return samples

def main():
    ensure_dir(TEST_DATA_DIR)
    write_wav("sine_1khz_0dbfs.wav", generate_sine(1000.0, 1.0, 0.0))
    write_wav("sine_1khz_minus18dbfs.wav", generate_sine(1000.0, 1.0, -18.0))
    write_wav("impulse.wav", generate_impulse(0.1))
    write_wav("sine_sweep_20_to_20k.wav", generate_sweep(20.0, 20000.0, 2.0, -6.0))
    print("All test calibration signals generated.")

if __name__ == "__main__":
    main()
