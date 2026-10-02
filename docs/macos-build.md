# Building the desktop sampler on macOS

The desktop sandbox is designed to build natively on both Apple Silicon and Intel Macs.

## One-time requirements

1. Apple Command Line Tools:

```bash
xcode-select --install
```

If macOS reports they are already installed, continue.

2. CMake.

With Homebrew:

```bash
brew install cmake
```

If you do not use Homebrew, install the macOS CMake application from cmake.org and make sure the `cmake` command is available in Terminal.

Raylib does **not** need to be installed separately. CMake downloads the pinned Raylib 5.5 source during the first configuration and builds it as part of the project.

## Easiest build

Open Terminal, change into the repository folder, and run:

```bash
./scripts/build-macos.sh
```

The script configures the project, compiles it, runs the sampler core tests, and opens the resulting `fm1_desktop.app`.

If macOS says the script is not executable:

```bash
chmod +x scripts/build-macos.sh
./scripts/build-macos.sh
```

## Manual build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
open build/fm1_desktop.app
```

## Using v0.2

- Drag a **16-bit PCM WAV** onto the window.
- The file is split into 24 equal slices.
- Trigger slices with the two keyboard rows shown on screen, or click the pads.
- Click in the waveform to move the selected slice start.
- Hold Shift and click the waveform to move its end.
- Up/Down changes the selected slice by one semitone.
- Space retriggers the selected slice.
- `R` restores 24 equal slices.

This is intentionally an early workflow prototype. It is not yet FM-1 firmware and it does not write to the synth.
