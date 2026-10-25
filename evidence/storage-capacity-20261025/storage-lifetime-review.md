# Independent storage lifetime review

Reviewer: bounded independent agent /root/storage_review. Read-only inspection of source e75fb2a584aa22978f352697a5e36fc9f886c3c5 in task-95/repo. No source edits, devices, GUI, network, builds or publication. This is a design review, not candidate implementation acceptance.

## Decision

Conditional GO for consolidation of native project io_buf with storage st_buf, with separate bounded streaming validation scratch. The current source is a single-main-loop, non-reentrant storage caller graph. A naked alias is NO-GO: st_save's initial st_current overwrites its incoming source, and final st_body readback overwrites it again. Both paths must cease writing the shared payload during saves. This does not authorize removal of independent editor/OTA/audio scratch or global serialization of formerly concurrent requests.

## Source evidence and lifetime proof

- project.c:83 owns io_buf. Its producers and consumers are synchronous save_obj/load_obj/load_settings, save_stream/load_stream and legacy migration. st_put and st_get are callbacks executed by lz_compress/decompress before those calls return; no callback or pointer escapes into a later main-loop iteration.
- project_save (754 onward) completes measure/settings/sound/stream synchronously. Each st_flush completes storage before reuse. read_project similarly completes every object and decompression before return. io_buf contains no live project or undo baseline.
- The only other plat_store callers are panel_init and panel_setup in panel.c:63,130. panel_init runs during boot, before project_load; panel_setup runs after project_load but before entering the ordinary main loop. Panel setup's wait loops cannot overlap an outstanding project load/save: the project call has returned. Panel payload remains independent RAM in .noinit.
- plat_fm1.c:184-185 dispatches synchronously to st_load/st_save. st_read, st_erase and st_prog in x0x.c:72-103 do not run UI/editor services or yield to the main loop. st_read limits IRQ-off windows to 256 bytes. fl_write limits each program to a page and parks core1 for every flash-off window. Do not widen those windows.
- main_fm1.c:84-179 audio and TIMER5 handlers call render, USB/BLE polling and input scanning; no project/storage calls occur. engine.c, BLE sources and HAL contain no calls to plat_store, project save/load or panel setup. The interrupt/core1 paths must remain unable to borrow this scratch.
- ed_service is called once from the main loop (2226 onward), separately from UI frame processing. editor.c retains project transfer state in ed_project.header, project_stage, ed_smp_buf and sample manifest fields, not io_buf. Project import/export can persist for many main-loop iterations without owning io_buf.
- ui_project_begin/end maintain project_stage/undo/audio ownership independently. Autosave/save is blocked during project_busy; import ABORT, malformed frames, timeout and recovery hold need no io_buf contents. OTA/PERF/LZ scratch is already a different 4096-byte allocation and must not be included in this consolidation.

This proof applies to this exact caller graph; any later asynchronous storage API, nested editor service during flash calls, ISR storage request or retained io_buf pointer invalidates it. Document the non-reentrant main-loop ownership at the shared allocation/interface. A busy flag only inside st_save/load cannot protect io_buf while project compression is filling it; do not claim that such a flag proves ownership.

## Required preservation constraints

1. Incoming payload remains byte-for-byte stable from entry to save through initial old-copy CRC selection, every program and final readback. Copy independent/XIP sources into RAM before erase; shared payload needs no self-copy. Streaming validation must use its own bounded scratch, not any slice of the incoming payload.
2. Keep A/B ordering, A tie winner, current seq arithmetic, fallback from corrupt newest body, header-last commit, page program boundaries, protected address allow-list and existing error propagation. No schema/header/object-ID/sample-map changes.
3. Ordinary failed st_load must leave external dst unchanged. Retain full staging for loads; do not stream directly into arbitrary destinations before CRC success. A shared internal scratch destination can be invalid on error only if every caller discards it and this is explicit; live project/stage/sample destinations cannot inherit that exception.
4. No new payload-sized stack buffer. Any verification chunk must have a fixed compile-time bound, recorded native frame/call-chain implications, and preserve the existing audio/core1 boundary behavior.
5. Native net XIP costs count: only 68 bytes of current app margin exist. A RAM-saving design that cannot fit is not an accepted candidate. Keep pool reserve at least 8192 bytes.

## Required production-linked verification

- Compile actual storage.c and project.c against the same NOR harness and native-sharing definitions; host fallback with separate arrays alone does not test aliasing.
- Fill a maximal shared payload with a distinct pattern, create older A/B content, save it and assert source unchanged after success and every failure. Exercise 0,1,255,256,257,3839,3840 lengths, too-long input, both copies empty, seq ties, malformed len/type/header CRC and corrupt newest/older bodies.
- Sweep read, erase, each payload-page program, header program and readback failure boundaries; include partial read/program mutations. Reboot by resetting volatile state and assert the valid committed old/new copy wins, external failed-load dst unchanged, and all bytes outside the target pair unchanged.
- Differential baseline/candidate deterministic successful operation traces should yield identical flash headers/payloads and returned bytes. Changed read chunk counts should be normalized when injecting faults by semantic boundary rather than assuming identical call counts.
- Interleave complete independent object/panel operations before and after project save/load/stream calls. Do not inject an impossible nested storage call from a simulated ISR and call refusal a production proof. During flash hooks, run actual relevant audio/input production paths where harness permits and assert scratch/source stable.
- Reuse production project stage/protocol/runtime/SMC runtime tests for malformed/truncated files, import abort, timeout, failed load retaining live project and undo, sample recovery hold, no sample writes, queued saves, and busy-input rejection. Any alias coverage must route the tests through the shared native allocation.
- Independent final review must inspect actual candidate diff and test failures/results after implementation; this design review alone is insufficient.

No action was denied, pending or uncertain. Baseline native builds and candidate measurements are outside this review. No delivered savings are asserted.
