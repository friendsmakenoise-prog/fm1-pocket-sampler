# FM-1 B-Boy Edition

Experimental desktop-first sampler/groovebox firmware project targeting the M-VAVE FM-1.

## The idea

Treat the FM-1 like a tiny late-1990s sampler that happens to contain an FM synth:

- three independent sampler tracks: **SAMPLE A / B / C**
- up to **24 chops per sample track**
- shared hardware-minded workflow rather than a DAW-style UI
- master trim before chopping
- manual/lazy punch chopping
- per-track mono/poly playback
- destructive/resampling ideas later to work around the FM-1's tight memory budget
- FM synthesis retained as an additional sound source for the later sequencer

The Akai MPC-style chop workflow is a reference point, but the interface is being redesigned around the controls actually available on the FM-1.

## Current milestone: v0.2.4 B-Boy Edition

The desktop sandbox now includes:

- orange FM-1-inspired front-panel skin
- `M-VAVE FM-1` boot screen with animated **B-BOY EDITION** spray reveal
- WAV / MP3 / FLAC / OGG desktop import
- 3 sample slots/tracks selected by the final three physical keys
- independent sample, trim, chop map and mono/poly state for A/B/C
- master START / END trim before chop generation
- 8 / 16 / 24 / manual chop modes
- manual punch-in markers while the master-trim preview is playing
- auto-centred hardware zoom around the currently selected trim point or chop
- linked slice-length editing
- per-track MONO/POLY toggle
- per-slice start/end, tuning and level
- real-time mixing of the three sampler engines

## macOS quick start

One-time requirements:

```bash
xcode-select --install
brew install cmake
```

Then from the repository folder:

```bash
rm -rf build
./scripts/build-macos.sh
```

The script explicitly uses the active macOS SDK, builds the app, runs tests and opens the resulting `.app`.

## FM-1-style control map in v0.2.4

- **MASTER** — output level
- **SELECT** — switch between MASTER TRIM and CHOP EDIT
- **PRESETS** — 8 / 16 / 24 / MANUAL chops
- **ALGORITHM** — zoom; automatically centres on the active trim point/chop
- **K1 / K2** — master start/end in TRIM, slice start/end in CHOP
- **K3 / K4** — slice tune / level in CHOP
- **OCT-** — linked slice length on/off
- **OCT+** — current sample track MONO/POLY
- **PLAY/STOP** — audition the current master-trimmed region
- **REC** — arm manual punch chopping
- first **24 keys** — chop triggers
- final **3 keys** — SAMPLE A / B / C

Desktop conveniences such as mouse waveform editing and right-drag panning remain available for development, but no hardware workflow depends on them.

## Status

This is still a desktop simulator, **not flashable FM-1 firmware**. Hardware firmware work should wait until the recovery/unbrick route and memory map are fully understood.
