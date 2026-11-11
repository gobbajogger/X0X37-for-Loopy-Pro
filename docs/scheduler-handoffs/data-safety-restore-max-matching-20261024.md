# Data Safety — complete sample slot migration matching (2026-10-24)

Owner: LoopyX0X37 Data Safety. Parent: firmware first NIGHTLY candidate PR #103 at full-root HEAD `94c093e01680d69e22b64a176b3dc27b7fffdd7f`, tree `c95bf58439d4b5fc3a438dc91cbebc97f7ecae2b`, compiled `firmware/` subtree `d3b67bf127121acdbb2d056890b8c6987ffb363e`. Consumed baseline `tools/x0x_slots.py` blob `2d62f8aa3ab5615fc2680b7a9de858e895ed81f6` and source-window hash FNV-1a64 `ab4138f9a779a35b`.

## Reproduced failure

The original `plan_migration` assigned sample images to the first fitting slot in old-slot order. With two incoming images of 48 B and 108 B and destination capacities [120 B, 64 B], it put the small image in the 120 B slot and **incorrectly skipped the large image**, despite the complete legal placement (small→64 B, large→120 B). The installer therefore could produce an avoidable partial restore after firmware update. This was a host-planner finding; no user-device loss was observed.

## Bounded correction

The planner now computes a maximum-cardinality, deterministic, safe matching of at-risk images to available destination slots, using most-constrained-first placement and reassigning occupants when necessary. It still never chooses kept or occupied slots, checks size before assigning, prefers original slot IDs in ordinary unconstrained cases, returns results in original risk order, and reports genuinely unplaceable archives as skipped. The write/readback, OTA, installer and firmware/controller code is untouched.

Modified: `tools/x0x_slots.py`, new `tests/x0x_slots_migration_matching_test.py`. The test file's Git blob is `f02aab647564ee96e70c45ed6bafb31471255e2a`; modified tool Git blob is `78b041ab3e0c736517dbf2f509c9776c7c632f10`. The patch changed only the exact 943-byte baseline function window to the 1729-byte corrected window (FNV-1a64 `d53582c532f45edc`). The changed Python tool preserves POSIX mode 100755.

## Actual local host checks

A private Library PR71 complete Python file was used as the executable host fixture. Its **entire original plan_migration function window** was independently byte-identical to the current PR #103 Git window; unrelated earlier/source code in the fixture is NOT claimed equivalent to PR #103. Replacing precisely that window and executing the same regression gives:

- `python3 x0x_slots_migration_matching_test.py x0x_slots_full_source_before.py` → **exit 1, 9 tests, 4 FAIL (expected baseline negative)**. Local log SHA-256 `f8dd429fa0a44de74847e25cda0ece030bf95babfec370af2302e6836bc53aee`.
- `python3 x0x_slots_migration_matching_test.py x0x_slots_full_source_after.py` → **exit 0, 9/9 PASS**. Local log SHA-256 `b50bf60e9dc71fcd1e22b26f26ec572ebe510f35edeb9476ec0a94fb96c4ab89`.
- `python3 -O x0x_slots_migration_matching_test.py x0x_slots_full_source_after.py` → **exit 0, 9/9 PASS**, same log SHA-256.
- `python3 -m py_compile plan_migration.after.py x0x_slots_migration_matching_test.py` → **exit 0**.

Coverage: positive complete remapping; forbidden kept and occupied destinations; same-index continuity; no destination and oversized image; preservation of original plan ordering; 250 deterministic randomized positive/negative/boundary matrices checked against brute-force maximum matching. Tests do not access MIDI, controllers or flash. Full exact current-root Python test suite, independent QA, target build, linked OTA, and hardware were **not** run.

## Exact handoff

Source ownership: Data Safety on one isolated stacked branch. **Independent QA**: execute committed new regression and existing `tests/install_slots_test.py`, `tests/x0x_slots_test.py`, restore/OTA, wrong-model/controller/CRC negatives from the exact remote full root; review maximum-cardinality behavior with representative real slot layouts. **Hourly Build**: the changed host Python does not change the `firmware/` compiled subtree. BUILD_ONLY from this exact source is allowed immediately; compiler-input equivalence, pinned SDK/toolchain, target packages and controller evidence are independently owned. **Sole Safe Merge**: reconcile this completed source PR with first candidate #103 only after the applicable independent checks. **Catalog/Publisher**: retain NO NEW INSTALLABLE status until prehardware safety, model, integrity, source/license and immutable HTTP proof.

Producer checks are not QA approval. There was no device test, owner hardware observation, protected-branch write, firmware/site publication, or installer activation.
