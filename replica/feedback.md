# User Feedback Analysis: Modern Audio Mastering Suites
Generated via `replica-entrepreneur` research workflow

## Sources Surveyed
- Public audio engineering forums (r/audioengineering, r/edmproduction, Gearspace, VI-Control, KVR Audio)
- Professional product reviews and community discussions on mastering tools (Ozone 10/11, FabFilter Pro-L2/Pro-Q3, T-RackS)

---

## Ranked Themes

### 1. What They Hate (Complaints about Incumbents)
1. **Installer & Authorization Bloat (Rank 1, High Impact)**:
   - Users universally despise having to install separate launcher daemons (e.g. iZotope Product Portal, Native Access, PACE iLok background processes) that run constantly in the background, consume RAM, and periodically fail offline authorization checks during live gigs or remote studio sessions.
   - *"I just want a simple VST3 DLL/folder that I drop in and it works without a 4 GB installer and background daemons running."*
2. **CPU Heavy / Latency Spikes (Rank 2)**:
   - High-end maximizers with inter-sample peak detection and multi-band dynamic EQ frequently spike DAW audio threads, especially during buffer changes or loop wraps in projects with high track counts.
   - *"Ozone chokes my session if I try to master in the mix project; I'm forced to bounce stems and master in an isolated session."*
3. **Black-Box "Magic" Assistant (Rank 3)**:
   - Many producers feel modern AI mastering assistants make decisions without explaining the acoustics or showing the exact transfer function adjustments, leaving engineers unsure of whether the low-end phase was compromised.
   - *"The assistant just slaps on a generic preset and hides what the dynamic EQ is actually reacting to."*

### 2. What Is Missing (Features Desired)
1. **Zero-Overhead Standalone + VST3 in One Lightweight Binary**:
   - Direct, instant loading (< 100 ms) with zero external background processes.
2. **Transparent, Explainable Auto-Master Recommendations**:
   - Clear numerical delta readouts: measured LUFS vs target profile, crest factor compression targets, and exact dB difference curves for Match EQ.
3. **Built-in Zero-Latency Phase & Mono-Maker Compatibility**:
   - A dedicated Mono-Maker and Haas stereoizer that mathematically guarantees 100% cancellation on mono playback.

### 3. What Is Unsolved (Positioning Opportunity)
- **The Lean, Precision Mastering Solution**:
  Producers who love Ozone's all-in-one mastering workflow but hate the bloat, sluggishness, and DRM overhead. They want an ultra-fast, zero-bloat mastering suite that runs natively in their DAW with under 3% CPU usage, strict ITU-R BS.1770-4 compliance, and transparent algorithmic assistance.
