# v0.2.6 — Shared Boundaries + Clean Checkpoint

## Sampler workflow

- Reworked the old LINK CHOPS behaviour into **LINK CHOPS**.
- In LINK CHOPS mode, internal chop markers are shared boundaries:
  - moving a chop START also moves the previous chop END
  - moving a chop END also moves the next chop START
- MASTER START owns the first chop start and MASTER END owns the final chop end while linked.
- LINK CHOPS can still be disabled for deliberately independent FREE start/end edits.
- Manual-mode shared markers are kept in sync when linked boundaries move.

## Repository cleanup

- Added `.gitignore` for `build/`, CMake output, macOS `.DS_Store`, compiled artefacts and editor-local files.
- Added `scripts/clean-git-tracking.sh` to remove the previously committed build tree from Git tracking and delete the local generated build tree.
- Restored/updated architecture, sampler design, roadmap, macOS build and memory-budget documentation in the clean distribution.

## Hardware guardrails

- Added an explicit FM-1 resource budget based on the currently mapped stock firmware.
- Marked 8 voices per A/B/C sampler as a desktop convenience rather than a firmware promise.
- Kept MP3/FLAC/OGG decoding explicitly desktop-side.
- Documented the need to avoid extra full-screen framebuffers and frame-by-frame boot-animation storage.
## Keybed hotfix

- Corrected physical/chop key 18 (A# in the second F-based octave) to use desktop key `4`, not `5`.
- The complete second-octave desktop map is now `Q 2 W 3 E 4 R T 6 Y 7 U`.

