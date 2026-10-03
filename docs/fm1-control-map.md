# FM-1 B-Boy Edition control map

The desktop simulator deliberately maps sampler and sequencer functions onto the physical controls available on the FM-1.

## Sampler / edit pages

| FM-1 control | Sampler function |
|---|---|
| MASTER | Main output volume |
| SELECT | Toggle MASTER TRIM / CHOP EDIT |
| PRESETS | 8 / 16 / 24 / MAN chop mode |
| ALGORITHM | Waveform zoom, auto-centred on current edit focus |
| OCT- | LINK CHOPS toggle; defaults ON |
| OCT+ | MONO/POLY toggle for current Sample A/B/C track |
| PLAY/STOP | Play/stop current track master preview |
| REC | Arm/disarm manual punch chopping |

### MASTER TRIM

| Knob | Function |
|---|---|
| K1 | Master sample start |
| K2 | Master sample end |
| K3 | Master tune |
| K4 | Reserved |

The first five white keys become PRE / GATE / 1SHOT / LOOP / TAIL audition functions.

### CHOP EDIT

| Knob | Function |
|---|---|
| K1 | Selected chop start |
| K2 | Selected chop end |
| K3 | Selected chop tune |
| K4 | Selected chop level |

With LINK CHOPS enabled, an internal chop boundary is shared by the two neighbouring chops.

## SEQ page — v0.3.0

| FM-1 control | Sequencer function |
|---|---|
| PLAY/STOP | Start/stop all sampler sequence tracks |
| REC | Arm live record/overdub |
| K1 | Active track length, 1–64 steps |
| K2 | Step edit cursor |
| K3 | Global tempo, 40–240 BPM |
| K4 | Event velocity |
| OCT- | QNTZ nearest-step / CURR current-step live recording |
| OCT+ | Mute/unmute active sequence track |
| SEL | Clear selected step |
| final 3 keys | Select sequence track A/B/C |
| first 24 keys | Place/audition chop; during REC+PLAY, record chop to grid |

All sampler tracks share one 1/16-note clock but have independent loop lengths. This permits conventional patterns and simple polymetric loops without storing audio tracks.

## Desktop shortcuts

- F1 / F2 / F3 — select A / B / C
- Space on SEQ page — PLAY/STOP
- Backspace on SEQ page — clear selected step
- QWERTY chop map: `Z S X D C F V B H N J M | Q 2 W 3 E 4 R T 6 Y 7 U`

Mouse waveform editing and right-drag panning remain desktop development conveniences; the intended hardware workflow does not depend on them.
