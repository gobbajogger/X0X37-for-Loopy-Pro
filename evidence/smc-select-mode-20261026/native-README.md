# SMC SELECT native qualification

Current tested source: `94061cf46483718bb63397d3c5fca8ca3d7ebb63`, tree `df01941b2bc136ac911632ae0c8de589405ee230`. Production SMC source blob is `5995318c34222d5909db90348552ea58484d6a66`.

Both official `tools/dev_validate.py` model runs completed as DEV CHECKED. Both records were then checked with the official `verify` command against the exact package and controller bytes. Final identities, log and artifact digests, memory limits and record locations are in `FINAL-QUALIFICATION.json`.

| Model | Code free | Static RAM free | Pool free |
|---|---:|---:|---:|
| Elite | 230508 B | 352 B | 21092 B |
| Pro | 230524 B | 352 B | 21092 B |

The compiler actually reported Clang 4.0.1. SDK commit is d179b4484759423312073f5fbb232501aa491047; all eight controller inputs matched maintained hashes and three application SDK file hashes were recorded. The immutable Linux/amd64 worker is sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1. Official records hash the complete SDK/toolchain and worker/runtime/configuration before and after each build.

The actual selected Linux SMC regression corpus contains 29 named selectors. Fifteen passed at bbb6eba6 with unchanged production/test closures. The original two failures were preserved: Solo's extracted harness lacked the new dependency, and reachability asserted the old HOME-as-FX behavior. The only bbb6→94061cf changes are those two harness files. Both corrected cases and all twelve unfinished selectors ran at exact94061cf and passed (fourteen total). `linux-94061cf-v2/receipt.json` and `checks.log` record that execution. `regression-reuse.json` and `verify_regression_reuse.py` bind the source mapping, original logs and current passing receipt. The official validation plan declares these as immutable dependencies and verifies their combined29 coverage instead of repeating all tests per model. No failed or unrun selector is counted as passing.

Initial runtime recovery is retained in failed attempt logs. The optional worker-side SDK Git metadata query was removed without a global trust exception; all SDK byte checks remained. The host bind filesystem did not support the parallel runner FIFO, so old serial attempts were preserved. The final corrected checks used an exact, container-owned Linux private copy with supported parallel FIFO execution. The obsolete failed-candidate processes were explicitly reconciled and only their identified PIDs were interrupted; their finally blocks removed their owned workers/worktrees. All final workers also cleaned successfully.

BUILD_ONLY results at bbb6eba6 are retained separately. The official current-candidate packages are byte-identical to those earlier outputs, now with genuine current-source validation records. No numeric version was allocated: inspection labels them Unversioned legacy DEV / DEVICE NOT TESTED. No physical MIDI, device command, flash, merge or publication was performed by this executor. Independent source/UI review and any integration/promotion action belong to the coordinator. Do not commit or upload the local .fwsc or ctl.img files; only sanitized records/logs/hashes are eligible evidence.
