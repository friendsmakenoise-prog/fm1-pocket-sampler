# v0.3.0 — Sequencer Foundation

The B-Boy Edition now has its first real groovebox sequencer.

## New

- three simultaneous sampler sequence tracks: **A / B / C**
- shared 1/16-note transport clock
- independent **1–64 step** length per track
- step-by-step event editing
- live record / overdub while the sequence runs
- live record quantise to the **nearest step** by default
- optional `CURR` record mode, which writes a hit to the currently playing step instead of rounding to the nearest boundary
- per-step slice number and velocity
- per-track sequencer mute
- 16-step paged grid display with edit cursor and playhead
- SAMPLE A/B/C keys also select the active sequence track
- sequencer state moved into a small portable core (`Sequencer.h/.cpp`) with tests

## Sequencer controls

- **SEQ** — open sequencer page
- **PLAY/STOP** — start / stop all A/B/C sequence tracks
- **REC** — arm/disarm live record/overdub
- **K1** — active track sequence length (1–64)
- **K2** — step edit cursor
- **K3** — tempo (40–240 BPM)
- **K4** — event velocity
- **OCT-** — toggle live record mode: `QNTZ` nearest step / `CURR` current step
- **OCT+** — mute/unmute active sequence track
- **SEL** — clear the selected step
- **keybed 1–24** — assign/audition a chop on the selected step; during REC+PLAY, record performed chops
- **final 3 keys** — select sequence track A/B/C
- **Backspace** — desktop shortcut to clear selected step
- **Space** — desktop shortcut for sequencer PLAY/STOP while on the SEQ page

## Existing workflow refinements

- **LINK CHOPS now defaults ON** for all sampler tracks; it can still be disabled for free-boundary editing.
- the corrected F-based keyboard map remains in place, including physical key 18 on desktop key `4`.
- MASTER TUNE remains per sample track and sequenced chops inherit it.

## Scope

v0.3.0 intentionally sequences only the three sampler tracks. The FM synth track, swing, true microtiming, pattern chains/song mode and resampling remain later milestones.
