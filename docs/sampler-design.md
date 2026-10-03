# B-Boy sampler design

## Core workflow

`LOAD / RECORD -> MASTER TRIM -> CHOP -> PLAY -> SEQUENCE -> RESAMPLE`

MASTER TRIM comes before chopping. A user can capture a rough section from a longer source, trim it to the phrase that matters, then generate or punch chops only inside that region.

## MASTER TRIM audition

The first five white keys become contextual audition tools while MASTER TRIM is active:

1. **PRE** — hear the material immediately before MASTER START
2. **GATE** — play START to END only while held
3. **1SHOT** — play START to END once
4. **LOOP** — repeat START to END while held
5. **TAIL** — hear the material immediately after MASTER END

These controls exist specifically so trimming can be done by ear without leaving the page.

## Three sampler tracks

The final three FM-1 keys select **SAMPLE A / B / C**.

Each track has:

- its own source sample
- master start/end
- 8 / 16 / 24 / manual chop map
- selected chop and per-chop settings
- MONO/POLY playback toggle
- future track sequence data

A typical beat might use A for a drum break, B for a musical/vocal phrase and C for a second break, bass or percussion source. The FM synth remains a separate sound source for later sequencing.

## MONO / POLY

MONO is intentionally simple and track-wide:

- MONO ON: a new chop on that sample track cuts the previous chop on that track.
- MONO OFF: chops from that track can overlap up to the shared hardware voice limit.

No extra legato/choke hierarchy is required for the first hardware design.

## Chop modes

- 8 equal regions
- 16 equal regions
- 24 equal regions
- manual punch/lazy chop

Manual mode plays the master-trimmed source and drops markers when a chop key is tapped while REC is armed.

## Chop boundary model

The preferred editing model is a chain of shared markers:

`MASTER START | marker | marker | marker | ... | MASTER END`

With **LINK CHOPS** enabled:

- moving a chop START also moves the previous chop END
- moving a chop END also moves the next chop START
- the first START is owned by MASTER START
- the final END is owned by MASTER END
- the chopped region therefore stays contiguous, with no accidental gaps or overlaps

With LINK CHOPS disabled, START/END can be adjusted independently for deliberate gaps, overlaps or unusual playback regions.

## Editing

- K1 / K2 edit the selected boundary pair
- ALGORITHM zooms around the selected chop or active trim boundary
- selecting another chop recentres the current zoom level automatically
- K3 / K4 edit per-slice tune and level
- waveform mouse editing and manual panning are desktop-only development conveniences

## Future priorities

1. chopping ergonomics and trim accuracy
2. A/B/C sequencer tracks plus FM synth track
3. resampling/bounce
4. memory-efficient on-device sample format
5. small, purposeful FX set
