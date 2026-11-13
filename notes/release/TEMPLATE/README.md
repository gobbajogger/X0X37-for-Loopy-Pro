# x0x37 release package: {{SHORTSHA}}

Status: TEMPLATE — NOT FILLED. Not an install permission. Only Firmware Main Builder installs (agent-rules.md rule 7, 8).
Package only a hash with a gate PASS and a device PASS (AGENTS_AND_ROOMS.md, Release Packager / Merge Integrator).

## 1. Source
| Item | Value |
|---|---|
| Source branch | {{SOURCE_BRANCH}}  <!-- TO CONFIRM: wip/one-build (PR #728 step 7) or integration/all-in-one (not on remote at 2026-10-30 20:50 VLAT) --> |
| Exact commit SHA | {{SHA}} |
| Commit link | https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/{{SHA}} |
| Git tree (`git rev-parse {{SHA}}^{tree}`) | {{GIT_TREE}} |
| Gate tree key (ls-tree without *.md) | {{GATE_TREE}} |

## 2. Evidence
| Check | Reference | Must be |
|---|---|---|
| Gate PASS row | /workspace/x0x37-results/index.md, row: `{{GATE_ROW}}` | Result PASS, written by the gate script, tree = {{GATE_TREE}} |
| Gate result file | /workspace/x0x37-results/{{GATE_RESULT_FILE}} | 0 fail, 0 skip ({{PASS_COUNT}}/{{PASS_COUNT}}) |
| Device PASS | {{DEVICE_PASS_REF}} (PR comment / log path) | Same bytes as section 3 (registry rule 4) |
| Full host test log | logs/host-tests-{{SHORTSHA}}.log (copy of {{TESTLOG_SRC}}) | Full suite, no partial run |

A PASS or "go" in a message is not evidence (agent-rules.md rule 7).

## 3. Image files
| File | Size (B) | CRC32 | SHA-256 |
|---|---|---|---|
{{IMAGE_TABLE}}

## 4. Loop slot backups (before install)
Docs say only "The backups are in slot_backups/" (USER_PREFERENCES.md). Exact paths: TO CONFIRM.
| Slot | Content (owner) | Backup path | CRC32 before | CRC32 after |
|---|---|---|---|---|
| 1 | 3 imported loops | slot_backups/{{TO CONFIRM}} | {{ }} | {{ }} |
| 2 | 2 imported loops | slot_backups/{{TO CONFIRM}} | {{ }} | {{ }} |
| 3 | (none known) | slot_backups/{{TO CONFIRM}} | {{ }} | {{ }} |

## 5. Features in this build
From docs/FEATURE-REGISTRY.md (PR #723), registry @ {{REGISTRY_SHA}}. One row per feature in the build. Snapshot: source-snapshots/.
| Feature | Status | Best source (PR, branch @ SHA) | Test / QA evidence | HW-verified? | Blockers |
|---|---|---|---|---|---|
| {{FEATURE}} | in wip/one-build | {{PR / branch @ SHA}} | {{evidence}} | {{yes only with device test on same bytes}} | {{ }} |

Not in this build (PR #728 "Out of scope"): #685, #690, composition stack, SMC popup UI. {{UPDATE}}

## 6. Hardware checklist
Copied from PR #718 (docs/hw-checklists-20261030 @ b71815e) into hw-checklists/:
- one-build-integration-hardware-checklist-20261030.md (main, tasks 1-9, rollback section 4)
- engines-ui, freeplay-pad-optout, midi-queue-order, smc-dual-peer-reconnect, smc-first-boot-roles, smc-mixer-buttons, smc-mixer-pages, smc-one-peer-central
| Checklist | Result | Who | Time (VLAT) |
|---|---|---|---|
| {{file}} | {{PASS/FAIL/NOT_RUN}} | {{ }} | {{ }} |

## 7. Install notes (for Firmware Main Builder only)
1. Make sure the gate row in section 2 is for tree {{GATE_TREE}}.
2. Back up loop slots 1 to 3 (section 4).
3. Send the owner notice with the exact hash {{SHA}}.
4. Install with `fm1_install.py --commit {{SHA}}` and the guard `require_gate_pass.sh`. Run in the foreground or a watched screen session. Do not use the web installer.
5. Do not write the bootloader.
6. Read the image back. Compare each slot CRC with section 4. Tell the owner only when one does not match.
7. If a check fails: do the rollback in hw-checklists/one-build-integration-hardware-checklist-20261030.md section 4.

## 8. Changelog (ASD-STE100)
- {{Add: <what the user can do now>.}}
- {{Fix: <what was wrong>. Now <what it does>.}}
- {{Change: <old behavior>. Now <new behavior>.}}
Use short sentences. Use one instruction in each sentence. Use the active voice.
