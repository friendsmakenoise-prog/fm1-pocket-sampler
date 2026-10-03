# v0.2.7 — Master Tune

- Adds MASTER TUNE to K3 in MASTER TRIM.
- MASTER TUNE is stored independently for SAMPLE A/B/C.
- Master audition modes (PRE / GATE / 1SHOT / LOOP / TAIL) now respect the track master tune.
- All chop playback inherits the master tune automatically.
- Per-chop K3 tuning in CHOP EDIT remains available and is added on top of the master tune.
- This behaves like classic sampler pitch/speed: pitching down lengthens playback and pitching up shortens it. It is not time-stretch.
- Keeps the v0.2.6 LINK CHOPS implementation and the Key 18 desktop mapping correction (`4`, not `5`).
