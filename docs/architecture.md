# Architecture

## Goal

Keep the musical/DSP code portable so the same engine can run inside:

1. a desktop development host; and
2. the eventual FM-1 firmware.

## Layers

### Portable core

No GUI, filesystem UI, audio-device or FM-1 dependencies.

Planned modules:

- `SampleBuffer` — decoded/recorded sample data abstraction
- `Sampler` — slice playback, interpolation and voices
- `Sequencer` — events, timing, quantisation, swing
- `Mixer` — track levels/pan/mutes and resampling bus
- `Filter` — lightweight resonant filter
- `Envelope` — amp/filter envelopes
- `Compression` — future ADPCM codec

### Desktop platform

Responsible for:

- host filesystem
- desktop audio/MIDI
- development UI
- importing/exporting sample banks
- simulation of FM-1 memory limits

### FM-1 platform

Added after hardware validation. Responsible for:

- display
- knobs/buttons/keyboard
- MIDI
- ADC/DAC/audio transport
- flash/filesystem
- timers

## Audio assumptions for v0.1

- Internal processing: 32-bit float on desktop for development convenience.
- Target output: 44.1 kHz unless hardware testing proves otherwise.
- Initial sample format: 16-bit PCM WAV import, downmixed to mono.
- Initial interpolation: linear.
- Initial polyphony: 8 sampler voices.
- Maximum visible slice bank: 24 slices.

These are development defaults, not promises about final FM-1 resource use.
