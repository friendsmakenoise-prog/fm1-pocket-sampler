# Vendor research archive reference

The project owner supplied an archive containing official/public FM-1 support files for research.

The binaries are **not** committed to the repository. This file only records their names, sizes and SHA-256 hashes so future reverse-engineering notes can identify exactly which inputs were studied.

| File | Approx size | SHA-256 |
|---|---:|---|
| `FM-1.fwsc` | 684 KiB | `db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a` |
| `M-UPGRADE.dmg` | 56 MiB | `57a61a8aac12e004bc4cd0b0aa63342d2046f21b50a4e3fe80ea460cfc401081` |
| `FM1.dmg` | 58 MiB | `02005c80404b5f9fb55e9138f11cfdecd821e8014577e546a91d85b270f242d9` |
| `FM-1 MIDI EN.docx` | 67 KiB | `f7cb237e3c83b2ee416c07c3b55019a8eb6dcab118cb7b9df39070d894e8dd8a` |

## MIDI document notes

The supplied MIDI document describes:

- note-channel Note On/Off
- Program Change 0-127
- Pitch Bend
- channel aftertouch
- Mod Wheel CC1
- Sustain CC64
- a separate FX channel (default MIDI channel 2)
- FX CC assignments 0-23
- MIDI clock/start/continue/stop
- single-parameter SysEx writes using `F0 43 10 pp qq vv F7`

These behaviours are useful compatibility targets when the project reaches actual firmware integration.
