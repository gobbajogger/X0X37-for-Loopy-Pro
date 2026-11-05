# Lane C: storage/save/flash

DONE: local commit be168fc8ae429ce835f26ad4410103c57ff0f50f (PERF #6).
Base: b2b929f46f1b7868dfa569d0d0989f5ab91aa16e.
Branch: lane/optimizer-storage-20261106.
Author/committer: gobbajogger <[redacted-email]>.
No push. No ui.c edits. No device operations.

## PERF #6: project stream chunk copy

Applied /workspace/dnbox/optimizer/patches/06-project-stream-chunk-copy.patch
without changes to firmware/src/app/project.c. Replaced byte copies in st_put
with bounded memcpy calls. This reduces save assembly overhead for players.
The measure branch, object headers, flush order, capacity handling and error
return paths retain their existing code. Flash erase/program and readback
verification code is unchanged. The LZ sink supplies separate stack tokens or
project raw bytes, so its input does not overlap the object buffer.

ESTIMATE gains, not measurements: for a 128-byte literal callback that fits
one object, sink loop iterations fall from 128 to 1 (99.2% fewer); a callback
that crosses a boundary needs two iterations. One-byte control callbacks gain
nothing and may have additional setup cost. Total save latency, target code
size and target memcpy performance are NOT_MEASURED. Erase/program latency
is unchanged by design.

Pool/static RAM delta: 0 bytes. No new firmware buffers or allocations.
The source adds a temporary uint32_t; register/stack allocation is NOT_MEASURED.

## Validation

Source review: completed. git diff --check: PASS.
Added tests/host/project_stream_chunk_copy_test.py in the same commit.
The standalone harness extracts the actual production st_put and compares it
with the original byte loop over 5,760 combinations: first/subsequent headers,
full/almost-full buffers, all object indices, zero/short/large writes, several
callback sizes, measure/capacity overflow, and each injected flush failure.
It compares complete mocked object bytes, lengths, remaining buffer bytes,
flush counts, return codes and writer state. It does not exercise the NOR driver.
Run command for the authorized host-test owner:
python3 tests/host/project_stream_chunk_copy_test.py

Differential execution: NOT_RUN. Full host suite: NOT_RUN. Elite/Pro builds:
NOT_RUN. Device validation: NOT_RUN. AGENTS.md and the task routing skill require
builds and host tests in classic ChatGPT project threads and prohibit Codex
execution. No bit-identical runtime PASS or firmware qualification is claimed.

## Further optimization review

Reviewed project.c save/measure/hash paths, app/lz.c, crc32_nibble.h and storage.c.
No additional optimization committed. The measure-before-write pass prevents
partial project writes on capacity overflow; removing it needs stronger proof.
The shared CRC already uses a compact nibble table. A larger lookup table would
cost XIP space and needs timing evidence. FNV equality hashing and post-program
CRC verification remain intact. LZ match-loop bounds could be hoisted, but its
gain under the target compiler is unmeasured and it needs separate packing
identity coverage. No XIP-during-erase design was introduced.

Next action: the optimizer coordinator can run the new differential test and
applicable project/storage regressions in the authorized test environment,
then qualify the integration candidate. This lane performs no integration.