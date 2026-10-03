# Roadmap

## Phase 0 — portable sampler core

- [x] WAV PCM import core
- [x] slice model
- [x] pitch playback
- [x] multi-voice playback
- [x] mono/poly sampler mode
- [x] additive rendering for multi-track mix
- [ ] move master-trim/chop-map track state into portable core

## Phase 1 — B-Boy desktop sampler

- [x] real desktop audio
- [x] waveform display
- [x] WAV / MP3 / FLAC / OGG desktop import
- [x] 8 / 16 / 24 equal chops
- [x] manual punch chops
- [x] master trim before chopping
- [x] PRE / GATE / 1SHOT / LOOP / TAIL trim audition
- [x] shared-boundary LINK CHOPS editing
- [x] optional FREE independent boundaries
- [x] zoom and desktop pan
- [x] hardware-style auto-centred zoom
- [x] three independent Sample A/B/C tracks
- [x] final three keys as A/B/C selectors
- [x] per-track MONO/POLY
- [x] orange FM-1 UI skin
- [x] B-Boy Edition startup animation
- [x] repository hygiene / resource-budget checkpoint
- [ ] zero-crossing snap
- [ ] slice reverse / loop playback controls
- [ ] project save/load

## Phase 2 — sequencing

- [ ] 3 sampler sequence tracks + FM synth track
- [ ] live record / overdub
- [ ] 64-step patterns
- [ ] swing / quantise / microtiming
- [ ] pattern chain / song mode

## Phase 3 — resampling and character

- [ ] internal resampling/bounce
- [ ] compact filter / drive / delay toolkit
- [ ] character sample-rate modes
- [ ] hardware-oriented compressed sample-bank format
- [ ] desktop export/transfer tool for that format

## Phase 4 — FM-1 hardware

- [ ] validate exact recovery/unbrick process
- [ ] measure target heap/RAM headroom with stock/reference firmware
- [ ] establish safe flash regions for replacement sample storage
- [ ] benchmark sample voice pool against FM synthesis and FX
- [ ] map USB audio/MIDI routes needed for record/transfer
- [ ] port musical state + sampler DSP
- [ ] only then produce a flashable B-Boy Edition package
