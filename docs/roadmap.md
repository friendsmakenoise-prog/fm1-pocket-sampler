# Roadmap

## Phase 0 — foundation

- [x] Portable C++ project skeleton
- [x] 16-bit PCM WAV loader
- [x] Mono downmix
- [x] Slice data model
- [x] Equal chopping
- [x] Basic polyphonic playback engine
- [x] Linear pitch interpolation
- [x] Host-side unit test

## Phase 1 — useful desktop sampler

- [x] Real desktop audio output
- [ ] MIDI note input
- [x] FM-1-like 24-key mapping
- [x] Waveform overview
- [x] Manual start/end editing
- [ ] Manual slice insertion/deletion
- [ ] Chromatic single-sample mode
- [ ] Coarse/fine tuning
- [ ] One-shot/gate/loop controls
- [ ] Basic low-pass filter
- [ ] Attack/release envelope

## Phase 2 — beatmaking

- [ ] 16/32/64-step sequencer
- [ ] Swing
- [ ] Per-step velocity
- [ ] Per-step sample lock
- [ ] Pattern save/load
- [ ] Resampling bus

## Phase 3 — four-track concept

- [ ] Four sequencer/audio track model
- [ ] Mute/solo/level/pan
- [ ] Loop recording
- [ ] Bounce/resample workflow
- [ ] Storage reclamation after bounce

## Phase 4 — FM-1 hardware validation

- [ ] Identify exact unit/hardware revision
- [ ] Confirm flash/storage geometry
- [ ] Confirm LCD/control mapping
- [ ] Confirm audio input ADC path
- [ ] Confirm output path/sample rate
- [ ] Establish safe recovery procedure
- [ ] Build hardware abstraction layer

## Phase 5 — firmware

Only after recovery and hardware behaviour are understood.
