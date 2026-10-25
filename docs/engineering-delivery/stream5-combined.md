# Combined project stream 5

Reason: recorder and Miami each froze an incompatible `PROJ_STREAM=4`
(endpoint flags vs `MOT_SNAPSHOT` lane `0xfe`). Those layouts cannot share the
number 4.

Purpose: allocate **stream 5** as the sole current revision that carries both
features, and migrate both frozen stream4 payloads on load. Next save rewrites
as 5. Mixed stream4 (endpoints and snapshots together) is rejected. Stream 3/2
still reject both new features.

User effect: projects keep held-note release timing and selected-step sound
locks in one file format; older recorder-only or Miami-only stream4 files still
open; corrupt mixes do not partially commit.

Pins: REC tip `90397cf0`, Miami `d7f156ec`. Host gate only — not native
Elite/Pro fit. No flash, NIGHTLY merge, or catalog publish from this change.
