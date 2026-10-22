# Direct SMC source reproduction — 2026-10-22

## Scope

Cloud-only, development-only (`X0X_DEV_PADCAL=1`), source-level validation of
publication commit `c2e63feb11385b91a9dbd3b29b20a368ed948a5a`, whose commit metadata
preserves original source `81459573792a68edd02fa1ee5658796b55597ba0`.
This is not a release, installer, complete firmware build, or hardware qualification.
It does not merge the later security/feedback proposals or change pinned admission.

Source was materialized from verified Git blob objects into a source snapshot, not
represented as a checkout of the unavailable original Git object graph. Historical
evidence/handoff directories were excluded from that snapshot. Dependencies were
retrieved from the official toolchain/SDK sources and verified against the repository's
pinned SHA256 values. No SDK/toolchain binaries or captures belong in this report.

## Strict controller compile and link

Run `tools/studies/smc_reference/link_runtime.py` with verified `--toolchain`, `--libs`,
`--rf-tables`, `--system-lib`, and fresh `--out`, using:

```
--controller-roles central --avctp-extra-commands 0 \
--placement data-only-92k-ramload --high-cpu-bss --cpu-init-pool \
--no-classic-test-modes --no-classic-key-sniff
```

Both independent output-directory builds resolved successfully with **zero unresolved
symbols**. No unresolved-symbol suppression, SDK stubs, memory-limit relaxation,
feature deletion, or retained-artifact-check bypass was used. The runner explicitly
marks its ELF **NON-INSTALLABLE**.

| Region | Used bytes | Capacity bytes | Remaining bytes |
| --- | ---: | ---: | ---: |
| Controller primary | 93,604 | 94,208 | 604 |
| Read-only data | 7,272 | 8,192 | 920 |
| RAM-text load | 3,524 | 4,096 | 572 |
| Low RAM extent | 26,352 | 26,368 | 16 |
| High CPU BSS | 6,448 | 7,168 | 720 |

Loaded controller total: 104,400 bytes. The very small low-RAM margin is a link-layout
result, not evidence of runtime stack/heap safety or full-application fit.

All three padded controller images extracted through the existing `seal_segments.py`
exactly reproduce the committed retained-vendor region pins:

- Primary: `c319dc7d58873ca16c90e4970e79f99d490b99a31b93a84e06d79c18687718f4`
- Read-only: `93bab7e70f040c006503eb9b2ed9d2a59451a213e9ed912c67e05aaed4d89db5`
- RAM-text: `0905950ad661147aa5de2cc1a16ae39f252814af8a48a7065c8d3915f8e5f0ed`

## Whole-file reproducibility limits

First new ELF SHA256:
`cf79ab0978292e6e3ed84b60ea9b9a8a84feb3bdf00b953c8fcb493edbe0c5c0`.
Second new ELF SHA256:
`61eb801e0b01ff87948c44ea83247ab00143fd40ea2da1c0a8e310180740859e`.
Neither is the historically admitted whole ELF (`7f3d21742a33d24ca326026ab7eb943fa6fbd73ddb2ec6072d92384735a7976d`).

Section-by-section comparison of the two new ELFs shows **only `.debug_line` differs**;
all loadable sections, symbol tables, and other debug sections are byte-identical.
`readelf --debug-dump=rawline` identifies two changed DWARF Advance-PC operands:
168 versus 178, and 234 versus 224, affecting a debug-only location around
`0x02084c72`. The executable bytes are identical. This demonstrates non-deterministic
debug output in these two builds; without the historical ELF bytes it does not prove
the cause of that historical ELF's difference.

The new result records differ only under `private_artifact_hashes`; ELF, map and
objdump-report hashes differ. Output-directory names occur in the reports. The
existing DEV builder rejects the new record as unreviewed, as intended. No pinned
hashes have been modified, and no installable artifact has been produced.

## Host checks

The following direct-source checks passed:

- `tests/ble/check_smc_profile.py`
- `tests/ble/check_smc_dev_validation.py`
- `tests/ble/check_smc_update_entry.py`
- `tests/ble/run_ctl_vendor_scheduler.sh`
- compiled `tests/ble/smc_readback_test.c`
- compiled `tests/host/smc_vendor_ingress_test.c` under ASan/UBSan
- `tests/host/smc_vendor_dispatch_test.sh` against real app/UI/engine sources

The container cannot run LeakSanitizer under its ptrace restrictions. Sanitizer
checks used `ASAN_OPTIONS=detect_leaks=0`; leak detection was **not tested**. Two
pre-existing signedness warnings in `ui.c` appeared during the dispatch check.

## Reviewed test-fixture repairs

Two stale test files were repaired without changing production firmware:

- `tests/host/smc_clip_visibility_test.c` counts actual `ck_set` calls rather than
  a removed `mark_dirty` stub. All 138 visibility/edit/no-replay assertions pass,
  retaining the warnings-as-errors build.
- `tests/smk_apc40_test.py` now checks the current `clip_pad_note` press-time capture,
  stored-note release, held-entry suppression, and mode-exit release/reset. Its
  outdated updater-guard assertion now checks the combined project-busy/clip-mode
  guard and timer reset before update entry. No isolation assertion was dropped.

An independent reviewer inspected both repairs. Four negative controls (retargeted
release, missing press-time capture, removed clip update guard, missing held-entry
suppression) were rejected with AssertionError by the repaired APC check.

A separate combined replay passed **all 15 explicitly selected host suites**:
`smc-protocol`, `smc-fader`, `smc-buttons`, `smc-queue`, `smc-link`,
`ble-midi-receive`, `smc-mapping`, `smc-filter`, `smc-feedback`, `smc-part-select`,
`smc-clip-visibility`, `smk-apc40-map`, `smk-apc40`, `smc-connection-watch`, and
`smc-host-ingress`. Run configuration: `X0X_DEV_PADCAL=1`, serial host execution,
and `ASAN_OPTIONS=detect_leaks=0`. This explicit focused run is not a full gate;
package tests were not selected or run. The runtime source is unchanged by these
repairs and by the intervening installer-diagnostic/rule-archive commits.

## Original baseline and app sizing replay

The original BASE commit was reconstructed from its published exact tree, parent,
author/committer metadata, dates and original message. The reconstructed object hash
is exactly `816e0d3987bcb69be972cff2cd4df4469969f196`, not an alias or replacement
commit. Its tree is `74db8ca92ab208c3a8fd963ad02e5e60a509d3a8`.

All 635 source blobs selected from `assets`, `ble`, `docs`, `firmware`, `tests`, and
`tools` were exported and independently checked against that original tree's blob
hashes. The local object store is intentionally incomplete for unrelated historical
capture/handoff/web blobs and original ancestor commits. Git archive returned zero
and exported every selected source file, but emitted a missing historical
`audit/HANDOFF.txt` object warning while inspecting the incomplete tree; this is not
represented as a full-repository export or clone.

The existing `smc_app_data/prepare.py`, `prepare_admission.py`, and `prepare_bridge.py`
from the direct source were applied to that disposable source export. Official SDK
boot/config inputs all matched the normal builder's committed pins. The unmodified
study builds completed for both SMK-37 Elite and Pro with `X0X_DEV_PADCAL=1`
(the sizes below match for both models):

- App image: **521,428 bytes** (2,572 below the separately defined 524,000-byte primary
  DEV partition; this study itself uses the ordinary app ceiling).
- App `.data + .bss`: **98,100 / 98,304 bytes**.
- Pool: **335,484 / 344,064 bytes**.
- RAM text: **1,089 instructions, no calls**; no soft-double routines; MMIO check passed.
- Elite app ELF SHA256: `574562e4874719ca2c251f40fd87a8dd9cbe74f70b00b77139f54652a7d09f27`.
- Elite app image SHA256: `9148c3a96f43fc118070bb29f0ff661abc459a996a84f93763a28c8f567ca8c3`.

The Pro study also passed the same checks and emitted no package; this does not
extend the separate named Elite-only DEV-package route to Pro.

This is the repository's **NON-INSTALLABLE, unsealed app-data sizing study**, using
STUDY_ONLY semantics, not the named installable DEV_PACKAGE selection. No `.fwsc`
was emitted. The legacy study loader also compiled, but that is not the separately
paired, admitted loader. These measurements do not claim the complete current DEV
package or hardware behavior has passed.

## Remaining admission boundaries

A byte-identical machine-code region reproduction is useful evidence, not permission
to replace whole-file pins automatically. Any pin refresh or acceptance redesign
requires separate review and must preserve the recorded source/input provenance,
region-layout checks, and existing fail-closed behavior. The original loader-source Git object and admitted retained artifacts must be located
or legitimately reconstructed before
claiming the historical full DEV packaging path has been reproduced. This report does
not establish their absence from other user storage. Installer repair remains owned
by the separate active conversation.
