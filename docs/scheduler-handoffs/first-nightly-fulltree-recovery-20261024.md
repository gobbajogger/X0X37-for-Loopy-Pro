# First NIGHTLY: real build recovery and blocking full-root regressions

Fresh native Elite, Pro and controller builds completed from PR103 R18 compiled
inputs at dc4669083218f17f8cfb69069e7f32911a232b52. They reproduce the earlier
published private R14 hashes exactly. This is BUILD_SUCCEEDED_UNREVIEWED, not
DEV CHECKED, live Pages, release permission or hardware PASS. Exact bytes,
CRC-32, compiler tree, SDK/toolchain pins and measured budgets are in the JSON
receipt alongside this file. Private firmware/controller bytes remain outside Git.

## Repaired check

`tests/host/smc_host_data7_test.py` extracted `smc_midi_host()` without its new
`smc_is_solo_release()` dependency or overflow generation counter. On a complete
R18 checkout GCC failed before executing any of its 13 tests. The harness now
extracts the actual production helper and declares the host fixture counter.
No production firmware behavior changed. Original failure is retained in logs;
repaired native test: 13/13 PASS. ASan+UBSan also passed with leak enumeration
disabled because this container cannot enumerate the traced process in /proc.
This does not claim LeakSanitizer PASS.

After repair, the seven selected SMC/package/controller safety checks pass,
including the actual SMK package validation with the pinned SDK supplied.
Earlier install/slot/backup/readback negative checks passed on R18. This is
selected coverage, not full release or independent combined-tree QA.

## Remaining defects reproduced on full source

Data Safety PR114 R4, exact 1f8d5830c188600c754b8240be4ea52cfe952f4f:
`tests/host/ota_port_binding_test.py` passes 17/17, but complete-root
`tests/install_test.py` fails 5 cases and `tests/install_slots_test.py` fails
13 cases. The mocked real transport changes port names when it becomes the
loader and then returns. R4 pins an exact name and refuses those transitions.
Do not simply loosen binding to any same-version keyboard: that reintroduces
wrong-device firmware writes or destructive slot restoration. Data Safety must
resolve endpoint identity, preserve safe interrupted-update recovery and run
both complete suites, including multi-keyboard negatives. Hardware identity is
still not established by a MIDI model/version string or a port name.

The public NIGHTLY browser libraries still choose the first matching device
and restore to the first non-loader. Their complete OTA/restore/controller
path requires independent review and corresponding repair before first publish.
The separate candidate-renderer patch must not be misrepresented as that fix.

## Pipeline fix prepared separately

The NIGHTLY website's only renderer unconditionally rejected nonempty catalogs.
A separate qualified-candidate command is now prepared on its own review lane;
normal preview remains empty. It requires exact-source independent gate
receipts, measured budgets, matching local controller, legal/source notices,
accepted firmware nightly tree and fresh HTTPS byte/hash/model checks before
writing a local output site. No protected branch or public site was changed.

## Next owner and acceptance

Independent QA replays this harness repair and full-root R4 failures. Data Safety
owns the OTA correction. The sole Safe Merge integrator incorporates only
independently accepted repairs into PR103/nightly. The compiler inputs here
remain unchanged; avoid redundant rebuilds after host/docs-only changes when
complete input equality has been verified. Catalog/publisher must still establish
lawful immutable external hosting, written break-generator permission evidence,
independent browser QA and live served hashes. Prior owner hardware PASS is not
required for a qualified NIGHTLY test build; production retains that requirement.
