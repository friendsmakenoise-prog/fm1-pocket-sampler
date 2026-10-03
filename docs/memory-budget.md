# FM-1 B-Boy Edition — memory and storage budget

This is the resource guardrail for the project. It deliberately separates the comfortable desktop simulator from what can plausibly live on the FM-1.

## What the current reverse engineering tells us

The public V13 architecture map for the FM-1 identifies:

- a 1 MiB NOR flash image
- stock `app.bin` of about **569 KiB**
- stock static RAM of about **132.5 KiB** (`.data` + `.bss`) before stacks, heap and live audio/UI allocations
- a **340 KiB VM region** used by configuration/KV storage
- a **72 KiB USR region** used for user patch storage
- a 240 x 240 RGB565 display
- 44.118 kHz, 64-sample-block stock audio processing
- a 240 MHz JieLi AC791N/WL82-family target

Source/reference: `AL-255/FM-1-RE`, `docs/architecture.md`. These values describe the analysed stock firmware; they are not yet a promise of how much RAM/flash a replacement firmware can safely claim.

## Desktop representation is intentionally NOT the firmware representation

The desktop `SampleBuffer` currently stores mono samples as 32-bit floats:

| Desktop representation | Approx bytes/sec |
|---|---:|
| 44.1 kHz mono float32 | 176,400 |

That means three one-second desktop samples already require about 517 KiB for sample payload alone. This is fine on a Mac and impossible as the final FM-1 strategy.

The desktop format exists to make development and debugging easy.

## Candidate hardware sample formats

Approximate payload only, excluding small block headers/metadata:

| Format | Approx bytes/sec | 72 KiB duration |
|---|---:|---:|
| 44.1 kHz / 16-bit mono PCM | 88,200 | ~0.84 sec |
| 22.05 kHz / 16-bit mono PCM | 44,100 | ~1.67 sec |
| 44.1 kHz / 4-bit ADPCM | 22,050 | ~3.34 sec |
| 22.05 kHz / 4-bit ADPCM | 11,025 | ~6.69 sec |
| 16 kHz / 4-bit ADPCM | 8,000 | ~9.22 sec |
| 11.025 kHz / 4-bit ADPCM | 5,513 | ~13.37 sec |

The 72 KiB column is deliberately conservative because that USR region is the only obvious stock user-storage region. A full replacement firmware may be able to reclaim more flash, but we should not budget against the 340 KiB VM region or other stock areas until hardware tests prove that safe.

## Cheap features — keep them

The following consume little compared with audio payloads:

- master trim points
- 8 / 16 / 24 / manual chop markers
- shared chop boundaries
- three A/B/C track state records
- MONO/POLY flags
- slice tune/level metadata
- sequencer note/event data
- swing/quantise/pattern metadata
- PRE/GATE/1SHOT/LOOP/TAIL audition state

These are measured in bytes or low kilobytes, not hundreds of kilobytes.

## Amber flags

### Sample voice count

The desktop currently allows 8 voices per sampler engine, so three tracks can theoretically produce 24 sample voices. That is a desktop convenience, not a firmware commitment. The hardware version should probably use one **shared sampler voice pool** (initial hypothesis: 6-8 voices total) and benchmark it against the FM synth and FX load.

### Full-screen graphics

A single 240 x 240 RGB565 framebuffer is about **112.5 KiB**. Avoid duplicating full framebuffers or storing a frame-by-frame B-Boy boot animation. The boot artwork should be a compact bitmap/mask or procedural reveal using the stock/display buffer strategy.

### General-purpose codecs

MP3/FLAC/OGG support belongs in the desktop preparation tool. The FM-1 should receive one compact native sample-bank format rather than carrying multiple large decoders.

### Floating point

The stock synth is heavily fixed-point. The portable sampler DSP currently uses float/double because it is convenient on desktop. The firmware port should move hot audio paths to fixed-point/integer arithmetic where practical and benchmark before committing to a final design.

## Working hardware rule

Until the physical FM-1 is profiled, design new features as if:

1. **audio payload is scarce**;
2. **metadata is cheap**;
3. **CPU-heavy polyphony is not free**;
4. **desktop codecs/UI are not firmware dependencies**;
5. destructive trim/resample/bounce is a feature, not merely a compromise.

This keeps the B-Boy Edition architecture honest while leaving room to discover a larger usable budget later.


### Master tune cost

MASTER TUNE adds only one floating-point control value per sampler engine in the desktop/reference core (three values total for A/B/C). The eventual fixed-point firmware implementation can store the same state in a few bytes per track. It does not duplicate sample audio or materially change the memory budget.
