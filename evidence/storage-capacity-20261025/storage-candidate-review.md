# Independent candidate data-safety review

Reviewer: /root/storage_review; scope: bounded independent source review and host validation. Reviewed isolated branch capacity/storage-scratch-20261025 against e75fb2a584aa22978f352697a5e36fc9f886c3c5. No production source edits, device, GUI, firmware flash, Git writes or publication by reviewer.

Verdict: PASS for the reviewed storage behavior and lifetime consolidation, conditional on native Elite/Pro budget and stack evidence plus completion of the parent's runtime regression checks. No actual native savings claim is made by this review at this point. The lifetime proof is in storage-lifetime-review.md.

## Exact reviewed candidate files (SHA256)

- firmware/src/storage.c: 3c0556e19a0676494bdee49ea3a931f29c76116db9b4f3fcfafb80a7071de571
- firmware/src/app/project.c: b2ace183417fcd13f880fd934bfbc506f024d1438ee571fd20cb5b88e7daaf76
- firmware/src/x0x.c: 59a97352ee3f9e00687ee73984ebc4548c5c4b8a94212c69b3eddd397d5a5962
- tests/storage_scratch_test.c: 8bb718ac73e66a66a4eec16c8628eed9700cb15b20246a154b8ca35e22cec449
- tests/storage_bytes_test.c: 4c654a52a39036a4ac7650d725228620e738458e60bccd6edff17c5a696ecf71
- tests/host/project_stage_test.c: d04d2f66db3f98bd4932cf0bf35ef68c25a3e7bf4097368c62d5ebf0454e06ce

## Findings

1. No blocking data-safety defect found. st_save uses preserve=1 for both current-copy selection and final body readback. These paths touch st_verify[256] instead of the incoming st_buf[3840], including partial failed reads. The self-copy of the shared source is bytewise identical and does not invalidate it. Independent or flash/XIP sources are still copied into RAM before erase/program.
2. st_load uses preserve=0, keeping the prior full-body staging and CRC validation before copying into an ordinary external destination. Failed reads/CRC fallback do not partially modify that destination. The native internal scratch destination may be invalid after a failed load; project callers already reject failed lengths before using scratch. No live project, stage or sample destination is introduced into this exception.
3. CRC update convention is correct: the externally finalized prior CRC is inverted on each call, processed with the unchanged nibble polynomial, then inverted again. crc32_more(...,0) matches the old CRC; zero-length body still performs one zero-length read and checks zero CRC, as before.
4. Header layout, reserved bytes, schema, seq arithmetic/order/tie behavior, object addresses, erase/program boundaries and error returns remain unchanged. Newest-body corruption still falls back. Read-only verification now has more hook invocations for saves but none is larger than the existing native 256-byte IRQ-off read bound. No ISR code, core1 ABI or audio allocation changes.
5. Native macro sharing is narrowly located around the project include and undefined immediately afterwards. Host fallback keeps its original independent allocation; the stage regression deliberately defines PROJECT_IO_BUFFER st_buf, so the primary project/storage test exercises actual aliasing rather than only the fallback.
6. Static allocation arithmetic is 3840 removed minus 256 added = 3584 bytes before linker alignment. No payload-sized stack, VLA, malloc, recursion or new ownership state. This arithmetic is not native measured savings; actual sections and frames remain the required evidence.
7. Source lifetime is guaranteed by the existing synchronous single-main-loop caller graph, not by a runtime lease. The new comment documents that constraint. Future async/nested-main-loop/ISR storage callers or retained project scratch pointers require renewed ownership design. Existing independent editor/OTA/sample buffers are unchanged.

## Independently executed checks

- cc -std=c99 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined tests/storage_scratch_test.c; execution PASS: 163089 assertions, no sanitizer error. Covers shared incoming source on success and every injected hook fault, partial I/O, lengths 0/1/255/256/257/3839/3840, persistent load failure, unchanged failed external destination, protected ranges, newest rot, malformed length, ties and flash sources.
- Independently extracted old storage.c from exact e75fb2a and compiled tests/storage_bytes_test.c against old and current implementations. Current build defines STORAGE_SHARED. Both executed 441 successful interleaved object operations, including panel object. Full 1 MiB NOR images compare byte-identical: SHA256 04d9058a3d7c687cac97647c2effcdc84625fb3e5ab603749c08a0b7359a8b9a for both. The outputs are local /tmp/task95-independent/{old,new}.bin, not public deliverables.
- sh tests/host/run_project_stage.sh independently PASS: 24446 checks, 0 failures; actual storage/project/defaults/sample metadata with shared buffer.
- git diff --check PASS.

The fault sweep is hook-level software evidence, not physical power-cut testing or measured hardware timing. Successful byte traces establish unchanged emitted persistent bytes for the exercised operations, not universal hardware qualification. Parent owns protocol/runtime regressions and native builds. No denied, pending or uncertain writes occurred in this review; native/runtime acceptance evidence was pending when this review was written.
