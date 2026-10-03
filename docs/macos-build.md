# Building FM-1 B-Boy Edition on macOS

## Requirements

1. Apple Command Line Tools:

```bash
xcode-select --install
```

2. CMake. With Homebrew:

```bash
brew install cmake
```

Raylib is downloaded automatically by CMake.

## Build

From the repository folder:

```bash
rm -rf build
chmod +x scripts/build-macos.sh
./scripts/build-macos.sh
```

The build helper resolves the active macOS SDK with `xcrun`, verifies that libc++ headers such as `<cstdint>` can be found, then passes the SDK/compiler paths explicitly to CMake.

## One-time Git cleanup after v0.2.6

An early local `build/` directory was accidentally committed to the public repository. v0.2.6 adds `.gitignore` and a helper that removes generated artefacts from tracking and deletes the local build tree:

```bash
chmod +x scripts/clean-git-tracking.sh
./scripts/clean-git-tracking.sh
```

Commit and push the resulting removals once. Future builds remain local and ignored.

## Current controls

- Drag WAV / MP3 / FLAC / OGG into the app to load the selected SAMPLE A/B/C track.
- Final three FM-1 keys switch SAMPLE A/B/C; `F1`, `F2`, `F3` are desktop shortcuts.
- SELECT switches MASTER TRIM / CHOP EDIT.
- PRESETS cycles 8 / 16 / 24 / MANUAL chops.
- ALGORITHM zooms around the active edit focus automatically.
- K1/K2 edit master trim or chop boundaries depending on page.
- K3/K4 tune and level a selected chop.
- OCT- toggles LINK CHOPS shared-boundary editing.
- OCT+ toggles MONO/POLY for the current sample track.
- PLAY/STOP auditions the master-trimmed source.
- REC arms manual punch chopping.
- MASTER TRIM first five white keys: PRE / GATE / 1SHOT / LOOP / TAIL.

Mouse-wheel waveform zoom and right-drag panning remain desktop-only conveniences.
