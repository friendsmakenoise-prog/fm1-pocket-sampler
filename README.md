# FM-1 Pocket Sampler

Experimental desktop-first sampler/sequencer firmware project targeting the M-VAVE FM-1.

## Vision

A pocket FM synth that behaves like a forgotten 1990s hardware sampler:

- Load or record samples
- Trim and chop samples across the FM-1 keyboard
- Chromatic playback and slice mode
- Characterful pitch interpolation / reduced-rate modes
- Fast performance workflow with minimal menu diving
- Sequencing and, later, a compact four-track looper/recorder
- Portable DSP core shared between desktop simulator and FM-1 firmware

## Current milestone: v0.1 desktop sampler

Initial scope:

1. WAV import (mono/stereo PCM 16-bit)
2. Sample memory accounting
3. Up to 24 slices mapped to keys
4. One-shot/gate playback
5. Pitching with linear interpolation
6. Portable C++ sampler core
7. Desktop test harness before hardware integration

## Design principles

- Desktop first; hardware later.
- No hardware flashing until recovery/unbrick workflow is understood.
- Keep DSP independent from the UI and platform-specific I/O.
- Treat memory, CPU and display limits as design constraints from day one.
- Optimise for hands-on hip-hop sampling rather than DAW-style feature density.

## Repository layout

```text
core/       Portable sampler/sequencer/DSP code
desktop/    Desktop simulator and development host
firmware/   FM-1 hardware integration (added later)
docs/       Architecture, workflow and hardware notes
tests/      Host-side tests
```

## Build

Requires CMake 3.16+ and a C++17 compiler.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The first executable is intentionally a command-line test harness. A graphical FM-1 simulator will follow once the sampler workflow is stable.

## Status

Early experimental development. Do not flash any generated firmware to hardware yet.
