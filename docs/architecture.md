# Architecture

## Goal

Keep the musical/DSP code portable so the same concepts can run inside:

1. the desktop development host; and
2. the eventual FM-1 firmware.

The desktop simulator is allowed to be wasteful where that speeds iteration. The FM-1 port is not.

## Layers

### Portable core

No GUI, desktop filesystem UI or FM-1-specific dependencies.

Current modules:

- `SampleBuffer` — convenient decoded sample abstraction for the host
- `Sampler` — slice playback, interpolation, voices, mono/poly and additive mixing
- `Slice` — per-chop start/end/gain/tune/playback metadata
- `Sequencer` / `SequenceTrack` — compact 64-step A/B/C event grids, per-track length/mute and BPM timing math

Planned portable modules:

- `ChopMap` / sampler-track state — shared boundaries, master trim and track metadata
- sequencer swing / microtiming / FM-track event types
- `Mixer` — track levels/pan/mutes and resampling bus
- `Filter` / `Envelope`
- compact hardware sample decoder (candidate: block IMA ADPCM)

The current A/B/C `TrackState` still lives in the desktop host because the workflow is being prototyped rapidly. Before hardware porting, the musical state—not the Raylib UI—should move into portable core types.

### Desktop platform

Responsible for:

- host filesystem
- WAV / MP3 / FLAC / OGG decoding
- desktop audio/MIDI
- development UI
- waveform rendering
- future sample-bank conversion/export
- simulation of FM-1 memory/voice budgets

### FM-1 platform

Added after hardware validation. Responsible for:

- 240 x 240 display
- physical encoders/buttons/keybed
- MIDI and USB transport
- audio input/output routing available on the target
- flash/filesystem access
- timers/RTOS integration
- compact sample-bank playback

## Audio strategy

Desktop defaults:

- float32 mono sample buffers
- 44.1 kHz output
- linear interpolation
- up to 8 voices per desktop sampler engine

Firmware target direction:

- compact stored sample format, decoded in blocks
- small shared sample-voice pool across A/B/C
- fixed-point/integer hot paths where useful
- no dependency on desktop audio codec libraries
- preserve the FM synth as a fourth sound source where CPU/RAM permit

See `memory-budget.md` for the resource guardrails.


### Global track pitch

The portable sampler core carries a per-engine `globalSemitones` value. SAMPLE A/B/C each own one Sampler instance, so master tuning stays independent per sample track. Rendering combines `globalSemitones + slice.semitones` before calculating the playback increment. MASTER TRIM preview uses the same pitch ratio, keeping audition and chopped playback consistent.
