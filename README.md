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

## Current milestone: v0.2 graphical desktop sampler

v0.2 adds a real desktop sandbox:

- Native audio output through Raylib
- Drag-and-drop WAV loading
- Waveform overview
- 24 visible slices
- Computer-keyboard and mouse triggering
- Selected-slice start/end editing
- Per-slice semitone tuning
- macOS `.app` bundle build

The portable core still provides:

- mono/stereo 16-bit PCM WAV import and mono downmix
- 24-slice model
- 8-voice sampler playback
- one-shot/gate/loop groundwork
- linear interpolation

## macOS quick start

One-time requirements:

```bash
xcode-select --install
brew install cmake
```

Then from the repository folder:

```bash
./scripts/build-macos.sh
```

The first build downloads and compiles the pinned Raylib 5.5 dependency automatically. See `docs/macos-build.md` for alternatives and troubleshooting.

## Desktop controls

- Drop a 16-bit PCM `.wav` onto the app window.
- Trigger the 24 slices from the keys shown on the on-screen pads or click them.
- Click the waveform to set the selected slice start.
- Shift-click the waveform to set its end.
- Up / Down: tune selected slice ±1 semitone.
- Space: retrigger selected slice.
- `R`: reset to 24 equal chops.

## Design principles

- Desktop first; hardware later.
- No hardware flashing until recovery/unbrick workflow is understood.
- Keep DSP independent from the UI and platform-specific I/O.
- Treat memory, CPU and display limits as design constraints from day one.
- Optimise for hands-on hip-hop sampling rather than DAW-style feature density.

## Repository layout

```text
core/       Portable sampler/sequencer/DSP code
desktop/    Graphical desktop development host
firmware/   FM-1 hardware integration (added later)
docs/       Architecture, workflow and hardware notes
scripts/    Convenience build scripts
tests/      Host-side tests
```

## Build without the desktop GUI

For core-only development/tests:

```bash
cmake -S . -B build-core -DFM1_BUILD_DESKTOP=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

## Status

Early experimental development. Do not flash any generated firmware to hardware yet.
