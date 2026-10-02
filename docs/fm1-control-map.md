# FM-1 B-Boy Edition control map

The desktop simulator deliberately maps sampler functions onto the physical controls available on the FM-1.

## Always available

| FM-1 control | Sampler function |
|---|---|
| MASTER | Main output volume |
| SELECT | Toggle Edit sub-page: MASTER TRIM / CHOP EDIT |
| PRESETS | 8 / 16 / 24 / MAN chop mode |
| ALGORITHM | Waveform zoom, auto-centred on current edit focus |
| OCT- | LINK LENGTH toggle for slice edits |
| OCT+ | MONO/POLY toggle for current Sample A/B/C track |
| PLAY/STOP | Play/stop current track's master-trimmed source region |
| REC | Arm/disarm manual punch chopping |

## Parameter knobs

### MASTER TRIM

| Knob | Function |
|---|---|
| K1 | Master sample start |
| K2 | Master sample end |
| K3 | Reserved |
| K4 | Reserved |

Chops are generated only inside the master trim region.

### CHOP EDIT

| Knob | Function |
|---|---|
| K1 | Selected chop start |
| K2 | Selected chop end |
| K3 | Selected chop tune |
| K4 | Selected chop level |

## 27-key bed

- Keys 1-24: chop triggers / chop selection.
- Key 25: SAMPLE A.
- Key 26: SAMPLE B.
- Key 27: SAMPLE C.

Each sample track owns an independent source sample, master trim, chop layout and MONO/POLY state. The three tracks share the overall memory budget in the eventual firmware.

## Auto-centred editing

The hardware has no spare pan control in the sampler workflow. Therefore ALGORITHM zoom centres itself on the current edit target:

- MASTER TRIM: midpoint of the master region; moving START/END centres on the boundary being edited.
- CHOP EDIT: selected chop midpoint; moving START/END centres on that boundary.
- selecting another chop recentres the view without changing zoom level.

Mouse panning in the desktop build is only a development convenience.


## v0.2.5 MASTER TRIM keybed audition

While MASTER TRIM is selected, chop triggering is disabled so trim audition cannot accidentally enter CHOP mode. The first five white keys are contextual audition commands: PRE, GATE, 1SHOT, LOOP and TAIL. The final three keys continue to select SAMPLE A/B/C.

Desktop QWERTY chromatic mapping begins on F and mirrors the visual bed: `Z S X D C F V B H N J M | Q 2 W 3 E 5 R T 6 Y 7 U`.
