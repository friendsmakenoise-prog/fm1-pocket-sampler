# FM-1 B-Boy Edition

Experimental desktop-first sampler/groovebox firmware project targeting the M-VAVE FM-1.

## The idea

Treat the FM-1 like a tiny late-1990s sampler that happens to contain an FM synth:

- three independent sampler tracks: **SAMPLE A / B / C**
- up to **24 chops per sample track**
- master trim and master tune before chopping
- equal or manual/lazy punch chopping
- **LINK CHOPS on by default** for contiguous break slicing, with optional FREE boundaries
- per-track mono/poly playback
- three simultaneous sampler sequence tracks with independent pattern lengths
- FM synthesis retained as an additional sound source for the later sequencer
- resampling/destructive workflows planned to make the tight hardware budget part of the instrument

The Akai MPC-style chop workflow is a reference point, but the interface is being redesigned around the controls actually available on the FM-1.

## Current milestone: v0.3.0 — sequencer foundation

The desktop sandbox now includes:

- orange FM-1-inspired front-panel skin and B-Boy Edition startup reveal
- WAV / MP3 / FLAC / OGG desktop import
- three sample tracks selected by the final three physical keys
- master START / END / TUNE before chopping
- PRE / GATE / 1SHOT / LOOP / TAIL trim audition
- 8 / 16 / 24 / MANUAL chop modes
- shared-boundary LINK CHOPS editing, default **ON**
- optional FREE independent chop boundaries
- manual punch chopping, auto-centred zoom, per-slice tune/level and per-track MONO/POLY
- **three simultaneous A/B/C sequencer tracks**
- independent **1–64 step** length for each sequence track
- per-step chop and velocity editing
- live record / overdub
- live record quantise to nearest step by default
- per-track sequencer mute
- 40–240 BPM global sequencer tempo

## Sequencer quick start

1. Load/chop samples on A, B and/or C as normal.
2. Press **SEQ**.
3. Select A/B/C with the final three keys.
4. Use **K1** for sequence length and **K2** to choose a step.
5. Press one of chop keys 1–24 to place that chop on the selected step.
6. Use **SEL** to clear the selected step.
7. Press **PLAY/STOP** to run all three tracks together.
8. Arm **REC** while playing and finger-drum; `QNTZ` snaps each hit to the nearest step.

Sequencer controls:

- **K1** — length (1–64 steps)
- **K2** — edit step
- **K3** — BPM
- **K4** — velocity
- **OCT-** — `QNTZ` nearest-step recording / `CURR` current-step recording
- **OCT+** — mute active sequence track
- **SEL** — clear selected step
- **PLAY/STOP** — global sequencer transport
- **REC** — live record / overdub arm

Track lengths are independent, so A can loop at 16 steps while B uses 12 and C uses 7, all against the same tempo/grid.

## Sampler control map

- **MASTER** — output level
- **SELECT** — MASTER TRIM / CHOP EDIT
- **PRESETS** — 8 / 16 / 24 / MANUAL chops
- **ALGORITHM** — zoom, automatically centred on the current edit focus
- **K1 / K2** — master start/end in TRIM, selected chop start/end in CHOP
- **K3** — MASTER TUNE in TRIM; selected chop tune in CHOP
- **K4** — reserved in TRIM; selected chop level in CHOP
- **OCT-** — LINK CHOPS on/off
- **OCT+** — current sample track MONO/POLY
- **PLAY/STOP** — one-shot audition / stop for the master-trimmed source outside SEQ
- **REC** — arm manual punch chopping outside SEQ
- first five white keys in MASTER TRIM — PRE / GATE / 1SHOT / LOOP / TAIL
- first 24 keys in CHOP EDIT — chop triggers / selection
- final 3 keys — SAMPLE A / B / C

Desktop QWERTY mapping follows the displayed F-based bed:

`Z S X D C F V B H N J M | Q 2 W 3 E 4 R T 6 Y 7 U`

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

## Repository hygiene

Generated build output and macOS `.DS_Store` files are ignored by `.gitignore`. If an older checkout still has generated files tracked, run once:

```bash
./scripts/clean-git-tracking.sh
```

Then commit the removals shown in GitHub Desktop.

## Hardware-budget rule

Desktop import/decode is intentionally luxurious. The FM-1 target will not keep full samples as 32-bit floats or carry desktop MP3/FLAC/OGG decoders. Sequence events are tiny metadata and are intentionally designed to be hardware-cheap. See `docs/memory-budget.md`.

## Status

This remains a desktop simulator, **not flashable FM-1 firmware**. No firmware should be flashed until the recovery/unbrick path and target resource budget have been validated on the physical unit.
