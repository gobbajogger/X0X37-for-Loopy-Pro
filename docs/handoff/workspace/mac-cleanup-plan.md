# Mac cleanup plan for DnBOX / x0x37 work (Mac-mini.local), 2026-10-29 PDT
Nothing was deleted by this pass.

## Warning: concurrent deletion seen
While this pass ran, someone else deleted ~/Documents/x0x37/worktrees (about 2.4 GB, about 90 worktrees) and ~/Documents/x0x37/codex-lanes (about 23 GB, including miami-opt2/build at 17 GB and chicago-opt/build at 2.1 GB).
- Free disk space went from 125 MB to 45 GB.
- All committed branches had already been pushed, so no commits were lost.
- Uncommitted edits in these worktrees were probably lost: miami-chicago-live-features (7 files), chicago-fx-34c0 (5), chicago-arp-34c0, chicago-sdly-insert-756e, chicago-skware-subby-spectrum-756e (1 each).
- The hub repo x0x37-for-loopy-pro still lists 10 worktrees, so run `git worktree prune` there.

## Pushed (non-main branches, no force push; private-header and binary scans were clean)
Repo gobbajogger/x0x37-for-loopy-pro, from the hub repo:
- codex/break-feat 83cec78a, codex/chicago-opt 230cde40, codex/fmb-guards-slots 70b87343, codex/miami-opt 3939fcef, codex/miami-opt2 2d523006
- codex/mixer-ux 6c84db92, codex/perf-test 186a0449, codex/pr-merges f3be22be, codex/smc-port 2d223fbf, codex/ui-theme 4dfac82a, codex/web-site 540330dc
- cursor/plugin-parameter-port-bfa1 bc55ff15, fmb-guards-xfer 0ff43a8b, lane/dk-pro-303a-crash-20261026 2dfe47b2, lane/mb-smc-proven-retest-20261022 0154cf7f, pr168 0c5fc921, smc-ble-fix 60968330

Repo gobbajogger/x0x37-for-loopy-pro, from other clones:
- lane/mb-note-effective-edited 12d04267, lane/bass-ui-integration-20261026 88b9132a, codex/bass-selectors-integration-20261026 3c773a66
- lane/mb-published-integration-20261021 816e0d39, source/original-monitor-336d71d, source/original-docs-3f9f5f7, source/original-smp-replay-b9569ab
- feature/multi-smc-app-20261025 f8b9fa85
- These went to backup/ names because the original name already existed on the remote with different history:
  - backup/task-6-project-compat-firmware-codex-project-export-compat-fw-20261026
  - backup/LoopyX0X37-Audit-SMC-Elite-ab1694e-x0x37-lane-mb-audit-integration-20261021
  - backup/task-5-master-handoff-cursor-master-20261025
  - backup/work-tracker-coordination-lx-req-007-local-owner-20261025

Repo gobbajogger/x0x37-for-Loopy-Pro_nightly:
- qualify/pr38-on-main 9f90c197, fix/ec013-zero-slot-info fbe74f93
- backup/nightly-smc-connect-fix-local-65ef47f8. The local smc-mixer-bluetooth-connect-fix branch has split from the remote ac7b73b1, so it was pushed under this backup name.
- cursor/drop-distribution-license-gate was already on the remote.

## NOT pushed on purpose
- ~/Documents/Codex/2026-10-29/task/lane: branches engine-original-owner 5319f377 and main a800e8bc. They contain the 12 private *_private.h headers ("reconstruct private common source"). Keep this folder; it is active.
- ~/Documents/x0x37/x0x37: its origin is genkigenki/fm1-x0x and pushing is DISABLED. About 28 old lanes from Oct 16–21 (ble-a54…a74, lane/ux-*, lane/sb-*, etc.) exist only locally. All of them are inside ~/Documents/x0x37/303_freeplay_envelope (35 MB). Keep that one folder.
- Staged changes in the hub: evidence/final-wbl1-tip-packages (ctl.img + logs). These are build evidence only; similar evidence is already on GitHub in dd735533.
- AGENTS.md edits in validation_batch11_*, sb-padcal-wait: trivial.
- Small folders with no remote: Codex/2026-10-25/task-6/native-prep (main d9fd9bcb), Downloads/SMK37-*/install-source (1 commit each).

## Deletion candidates (sizes measured now)
| Category | Paths | Size | Safe (on GitHub / duplicate) |
|---|---|---|---|
| Old clones of ~/Documents/x0x37/x0x37 (ux_*, smc_*, chord_*, integration_*, firmware_*, rules_*, b3/b7/b10 repo, flash_review*, work-303, gate_*…). All their refs are also in 303_freeplay_envelope. | 51 dirs | ~854 MB | yes (duplicate), if 303_freeplay_envelope is kept |
| Codex task workspaces 2026-10-25, 2026-10-26, 2026-10-28 (task-6 3.5G, task-7 1.7G, task-646c45a5 887M…). Branches are pushed. | ~/Documents/Codex/2026-10-2{5,6,8} | ~13.1 GB | mostly yes. NO for task-67/recovery-store (193 uncommitted files), task-32/firmware (6), task-38/repo (6), task-8/native-prep (4), task-6/native-prep (1 local commit) |
| Old install/dev-check folders (*_install_202610{20,21,22,26}, dev-check-*, smc-connect-fix-80de61ac / 979dc017). Keep smc-connect-fix-ac7b73b1, which is the latest install. | 17 | ~283 MB | logs: yes; keep any slot backups inside them |
| Downloads build packages and zips (LoopyX0X37-*, SMK37-Elite-*, DnBOX-*, X0X37-*, *.fwsc, "(1)" duplicate zips), plus home x0x37-mac.tgz/.diff, dnbox-base-smc.bundle, x0x_trial | 54 | ~432 MB | yes (build outputs; the source branches are pushed). The SMK37-*-install-source dirs hold 1 local commit each, which is of low value |
| Empty Codex date dirs (04-24, 05-06, 06-09, 10-14, 10-21, 10-22, 10-27) | 7 | 0 | yes |
| Library/Caches | ~/Library/Caches | 3.5 GB | general caches, not DnBOX; optional |

Reclaimable now: about 14.6 GB (about 13.1 GB in Codex workspaces, about 1.6 GB in x0x37 clones, logs and Downloads). The concurrent deletion has already freed about 25 GB.

## Flagged separately: KEEP (device data)
- 271 slot backup / CRC records (*.x0xslots*, ~/Documents/x0x37/slot_backups), about 26 MB in total. Keep all of them; the space saving is tiny.
- ~/.config/x0x (ctl.img, ctl.img.abi2.bak) and private-audio (117 MB): keep.
