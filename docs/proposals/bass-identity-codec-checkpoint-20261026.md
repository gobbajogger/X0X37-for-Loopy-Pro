# Unintegrated bass identity checkpoint

Paused at owner direction while real engine/preset UI takes priority. This new-file unit has no
production consumer and changes no REC, sound, motion, storage, parser or UI fields.

`bass_identity.h` is an eight-byte LE codec with caller-supplied registry. Registry rows derive
engine IDs from existing backend declarations and counts from actual product descriptors.
Its product sound schema is distinct from DSP-only schemas: native303 DSP has23 parameters,
while current product Chicago dispatch exposes32; WBL1 product dispatch exposes23.
Unknown legacy revisions through10 always require explicit engine choice. No part-index,
value-range, snapshot or producer-version inference is made. Unknown future tags cannot be
re-encoded as canonical known tags; unavailable engines cannot be admitted as ready.

`python3 tests/host/bass_identity_exports_test.py` passed against two independently generated
actual stream3/sound10 and stream6/sound10 exports plus two CRC-valid synthetic Carrier48
variants:293 checks with WBL1 disabled,284 with it enabled. Entire export bytes and fixture JSON
remain unchanged. Fixture source hashes/provenance are retained; no user data is present.
ASan/UBSan, independent review, native proof and production integration are NOT RUN here.

The next shared-source unit would coordinate engine.h(sound_t),x0x.h(soundrev11),project.c
load/admission/save/file headers and the whole-project producer pair(6,11), plus browser fixtures.
REC1d2418a2 stream6 is endpoint-only and rejects frozen snapshot streams4/5 and used0xfe lanes;
its sticky-9 zero-write preservation must remain until an independently validated decoder exists.
No current schema or stored data is modified by this checkpoint.
