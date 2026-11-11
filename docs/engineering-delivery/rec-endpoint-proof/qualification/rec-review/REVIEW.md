# Independent REC endpoint review

Pinned head `6ddea31d6040d6735989e64adbe11630fc1ee7ec`, tree `c61706b9f06168c2500ff5f2d792a5f253ec478d`, base `d633833ac369f8466bc2563791614206070e87d1`. Production source left unchanged; exact commands, source blobs and log SHA256 in receipt.json.

## Disposition: one preservation blocker

`firmware/src/app/project.c`, `st_get` (around lines 286–290) loses unsupported format identity on a **consumed continuation segment**. `load_stream` and `read_project` then classify the result as ordinary corruption. This bypasses the new incompatibility save block.

Independent synthetic NOR reproduction:

1. Save a valid stream6 project large enough to consume OBJ_PAT1 (packed_len6679, OBJ_PAT1 length2859).
2. Change only OBJ_PAT1's format header to unsupported5, through real object storage. Object CRC remains valid; this is a mixed/torn stream, not a valid homogeneous stream5.
3. `project_load()` returns -1; `project_storage_unsupported()` is0.
4. Call the ordinary boot-default fallback `project_defaults()`, then `project_save()`.
5. Save returns0, with4object saves,4erases,26programs, overwriting the mixed stored bytes.

Expected: encountering a recognized unsupported format in a used segment retains the -9 incompatibility classification, prevents implicit fallback/autosave writes, and preserves the stored bytes until explicit validated reset/adoption. Do not reject unused trailing segment headers: a short valid stream may leave a stale tail legitimately. Reproduce with `SAN_FLAGS='-g -fsanitize=address,undefined' sh ../rec-review/mixed-storage-probe.sh`; this diagnostic deliberately prints actual state rather than failing its process. The homogeneous unsupported4/5 and snapshot paths already pass their zero-write regressions.

## Independently verified behavior

- All four new registered endpoint/lifecycle/storage/nonforward suites passed ASan+UBSan.
- Thirteen adjacent registered suites passed: both live REC fixtures, legacy2/3 storage compatibility, staging, sequencer, long-note/random-preview, bass MIDI probability, and REC/BON/BOFF/BRK/DRUM CQ overflow.
- Additional actual UI key-event probe passed: older physical key release cannot end the successor; successor release ends it; outgoing slide survives production replay. A stale queued keyboard REC epoch auditions without recording.
- Additional production-sequencer boundary matrix passed96cases/288assertions: BPM20/120/275, all4rates, swing0/100, FWD/REV, lengths3/4. Release error remained within a1/15-slot bin plus integer rounding.
- Source review confirms three-byte steps, endpoint upper nibble, generation/source ownership instead of DSP-gate ownership, atomic C_REC entry, unsupported direction refusal before pattern mutation, existing transport/audition preserved, staged validation, and explicit reset/adoption recovery boundary.

PINGPONG/RANDOM exact per-occurrence recording, historical per-voice engine identity/snapshot migration and selector UI remain outside this unit. No native fit, controller package, hardware, flash, publication or universal full regression claim follows from this review. Native qualification is a separate required gate after the preservation issue closes.
