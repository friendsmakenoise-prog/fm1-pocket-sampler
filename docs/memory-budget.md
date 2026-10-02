# Memory and storage budget

This document separates **working RAM** from **persistent sample storage**.

Exact FM-1 limits will be verified on the physical unit before firmware integration.

## PCM storage cost

Mono PCM recording approximate storage:

| Format | Bytes/sec | 1 MiB duration |
|---|---:|---:|
| 44.1 kHz / 16-bit | 88,200 | ~11.9 sec |
| 22.05 kHz / 16-bit | 44,100 | ~23.8 sec |
| 11.025 kHz / 16-bit | 22,050 | ~47.6 sec |
| 22.05 kHz / 8-bit | 22,050 | ~47.6 sec |

IMA ADPCM is a candidate later because it offers approximately 4:1 compression relative to 16-bit PCM with modest decode cost.

## Desktop constraint simulation

The desktop host should eventually expose a fixed target-storage budget (for example 2/4/8 MiB profiles) so we can assess realistic beat/sample workflows before committing them to hardware.

## Important distinction

The desktop v0.1 currently decodes WAV files to 32-bit float for simplicity. Final firmware will almost certainly use a more compact representation and/or streamed block decoding.
