# Sampler workflow design

## Product idea

A pocket FM synth that behaves like a forgotten 1990s hardware sampler.

The target user workflow is closer to Akai/SP-style beatmaking than a miniature DAW.

## Primary flow

`LOAD/REC -> TRIM -> CHOP -> PLAY -> SEQUENCE -> RESAMPLE`

### Load / Record

Sources eventually include:

- WAV import
- external audio input, if FM-1 ADC routing permits
- internal resampling from synth/sampler/mix bus

### Trim

Fast operations:

- Start
- End
- Fine start/end
- Normalise
- Reverse
- Fade in/out

### Chop

Modes:

- Manual chops
- Equal divisions
- Transient-assisted chopping (later, resource permitting)

Up to 24 slices are immediately playable from the keyboard bank.

### Playback

Each slice stores:

- start/end
- one-shot/gate/loop
- gain
- tuning
- envelope (later)
- filter settings (later)

### Character

Rather than hiding limited resources, lower-quality modes can become musical features:

- 44.1 kHz clean
- ~22 kHz lo-fi
- ~11 kHz crunch
- selectable interpolation behaviour
- optional ADPCM storage mode

## Performance controls

The exact knob mapping will be tested in the desktop simulator. A candidate sampler page:

1. Start
2. End
3. Tune
4. Filter cutoff
5. Resonance
6. Attack
7. Release
8. Level

Shift layers should expose secondary parameters without making basic chopping menu-heavy.
