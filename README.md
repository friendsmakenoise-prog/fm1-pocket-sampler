# FM-1 B-Boy Edition

Experimental desktop-first sampler/groovebox firmware project targeting the M-VAVE FM-1.

## The idea

Treat the FM-1 like a tiny late-1990s sampler that happens to contain an FM synth:

- three independent sampler tracks: **SAMPLE A / B / C**
- up to **24 chops per sample track**
- master trim before chopping
- equal or manual/lazy punch chopping
- **shared chop boundaries** for contiguous break slicing
- per-track mono/poly playback
- FM synthesis retained as an additional sound source for the later sequencer
- resampling/destructive workflows planned to make the tight hardware budget part of the instrument

The Akai MPC-style chop workflow is a reference point, but the interface is being redesigned around the controls actually available on the FM-1.

## Current milestone: v0.2.7 — master tune

The desktop sandbox includes:

- orange FM-1-inspired front-panel skin
- `M-VAVE FM-1` boot screen with animated **B-BOY EDITION** spray reveal
- WAV / MP3 / FLAC / OGG desktop import
- three sample tracks selected by the final three physical keys
- independent source sample, trim, chop map and MONO/POLY state for A/B/C
- master START / END trim before chop generation
- per-track **MASTER TUNE** before chopping; slice tuning remains relative to it
- MASTER TRIM audition tools: **PRE / GATE / 1SHOT / LOOP / TAIL**
- 8 / 16 / 24 / MANUAL chop modes
- manual punch-in markers while the master-trim preview is playing
- auto-centred hardware zoom around the active trim point or selected chop
- **LINK CHOPS** mode: moving a shared boundary updates both neighbouring chops
- FREE chop mode for deliberately independent start/end edits
- per-slice tune and level
- real-time mixing of the three sampler engines
- F-based desktop key mapping matching the visual FM-1 keybed

### LINK CHOPS behaviour

With LINK CHOPS enabled, chops are treated as contiguous regions separated by shared markers:

`MASTER START | S1 | S2 | S3 | ... | MASTER END`

Moving the start of S2 therefore also moves the end of S1. Moving the end of S2 also moves the start of S3. The first START and final END remain controlled by MASTER TRIM.

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

The build helper explicitly uses the active macOS SDK, builds the app, runs tests and opens the resulting `.app`.

## One-time repository cleanup (introduced in v0.2.6)

An earlier build directory was accidentally committed. After merging this update, run:

```bash
./scripts/clean-git-tracking.sh
```

Then commit and push the removals shown in GitHub Desktop. `build/` and macOS `.DS_Store` files are now ignored permanently.

## FM-1-style control map

- **MASTER** — output level
- **SELECT** — MASTER TRIM / CHOP EDIT
- **PRESETS** — 8 / 16 / 24 / MANUAL chops
- **ALGORITHM** — zoom, automatically centred on the current edit focus
- **K1 / K2** — master start/end in TRIM, selected chop start/end in CHOP
- **K3** — MASTER TUNE in TRIM; selected chop tune in CHOP
- **K4** — reserved in TRIM; selected chop level in CHOP
- **OCT-** — LINK CHOPS on/off
- **OCT+** — current sample track MONO/POLY
- **PLAY/STOP** — one-shot audition / stop for the master-trimmed source
- **REC** — arm manual punch chopping
- first five white keys in **MASTER TRIM** — PRE / GATE / 1SHOT / LOOP / TAIL
- first 24 keys in **CHOP EDIT** — chop triggers / selection
- final 3 keys — SAMPLE A / B / C

Mouse waveform editing and right-drag panning remain desktop development conveniences; the intended hardware workflow does not depend on them.

## Hardware-budget rule

Desktop import/decode is intentionally luxurious. The FM-1 target will not keep full samples as 32-bit floats or carry desktop MP3/FLAC/OGG decoders. The hardware plan is compact stored sample data plus small decode/mix buffers. See `docs/memory-budget.md`.

## Status

This remains a desktop simulator, **not flashable FM-1 firmware**. No firmware should be flashed until the recovery/unbrick path and target resource budget have been validated on the physical unit.
