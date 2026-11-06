# P2 continuation read classification: same-reviewer recheck

P2 isolated evidence packet; source integration remains unselected. Parent should return this packet to the existing independent reviewer who raised P2; no new reviewer or QA closure is claimed.

Base candidate tree: d93c60a7b21bb2cfa92ac40bc43cfbe470b5073d.
Revised tree: 3f8eb61bcba4915eef02f0b7fddb3bba23a6787a.
Patch: P2-continuation-read.patch (apply to the base tree).
SHA256: d338d020176043967b025d8fe5b43e6111c29a7268b672becffdfdf27f84aa5e.
Fresh-index application reproduced the revised tree exactly.

Independently reproduced P2 using occupied PAT1/SONG/MOTION objects, each with a current-copy header and payload I/O fault. New regression on unchanged production source: 27,010 checks, 18 failures, exit 1 (red.log). Failure is classification, not an observed unauthorized write: project_storage_unsupported becomes 2 and save returns -9. Staged load does not clear that incorrect latch; successful live load recovers.

Fix: preserve -2/-8 from the continuation reader through load_stream. Stop A/B retries on unavailable reads, including errors encountered during recovery, and mark the failed load transient. Damaged CRC/committed-data handling and rev13 rejection stay intact. Five added production lines in project.c; 24 added test lines in project_stage_test.c. No sequencer, SMC, storage implementation, M7a persistence/export or shared runner changes.

Actual commands on GCC 14.2, CC=gcc:
- sh tests/host/run_project_stage.sh: PASS exit 0, 26,864 checks (green.log).
- SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/run_project_stage.sh: PASS exit 0, 26,864 checks (sanitizers.log).
- sh tests/host/run_project_save_preflight.sh: PASS exit 0 (preflight.log).
- python3 tests/host/test_smc_boot_saved_roles.py: PASS exit 0, 28 checks each plain/UBSan (boot-roles.log).
- sh tests/host/project_runtime_test.sh: PASS exit 0, project and SMC runtime (runtime.log).
- git diff --check and fresh-index patch reconstruction: PASS.

The new cases assert no write/flash mutation during failure, retryable save -8, staging alone cannot authorize fallback save, and successful live load restores the exact project and permits save. Existing actual-damage, A/B recovery, M7a tail/live/staged/export assertions all execute in the passing stage suite. The original normal/sanitizer suite count was 26,600; the new green count is 26,864. Red count differs because obsolete recovery attempts cause additional storage assertions.

Full suite and native/device qualification NOT_RUN. Prior legacy format-2 migration failure on both original proposal and first candidate remains an existing issue, not fixed or relabeled PASS. Prior OFF cross-firmware proof retains its earlier exact-input scope; it was not rerun here. This packet is published under the later explicit evidence-handoff authorization. No shared source branch changes, merge, firmware publication or devices. Existing published evidence commit 95f76cafcfeef7ec2964aa5bf933b12058e65a73 still describes the prior candidate.
