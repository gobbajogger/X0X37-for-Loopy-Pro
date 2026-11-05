# Codex import — lost / local-only work

Source: 96 `rollout-*.jsonl` files → **94 sessions** (2 sessions split across 2 files).
Repo: `gobbajogger/X0X37-for-Loopy-Pro`. Tip checked: `wip/one-build` = `a06e20189` (= `a06e20189141`).
Times: Asia/Vladivostok (VLAD, UTC+10). No tokens/secrets included.

## Summary

- Sessions: **94**
- Finished cleanly: **76**
- Finished but blocked/limited: **10**
- Unfinished: **7**
- Aborted: **1**
- Sessions that wrote local patches: **20**

### What this archive actually is

Most threads are **optimizer / analysis Codex runs** on that Grok Bot box. Many claimed `DONE` with local commits.
GitHub check shows two different situations:

1. **On GitHub but not in tip** — lane branches exist, ahead/diverged from `a06e20189`. Work is not lost; it needs rebase/catch-up onto tip (PR Babysitter stalls).
2. **Never on GitHub** — local SHAs from Codex logs do not resolve via the API; themes have no matching commits. **This is true lost work** (recover from patches/notes below).

## Truly unpushed / missing on GitHub (top items)

| Item | Local SHA | Claimed branch | Patch / notes | In tip? |
|---|---|---|---|---|
| **USB cooperative reset (BEHAVIOR-CHANGING)** | `ab6b6ae7` | `lane/optimizer-latency-20261106` | `patches/lost-02-usb-retry-cooperative-reset.patch` | **NO — SHA missing** |
| **PERF #6 project stream chunk copy** | `be168fc8` | `lane/optimizer-storage-20261106` | `patches/lost-06-*.md/.py` (diff not fully retained; test+log recovered) | **NO — SHA missing**; GH storage tip is LZ-skip `c67eb8347`, not this |
| **Bus-FX call-invariant hoist** | `f8a015ae` + test `ea0d18b6` | `lane/optimizer-engines-20261106` | `patches/lost-03-busfx-call-invariants.patch` + `lost-03-busfx_equivalence_test.py` | **NO — SHAs missing**; GH engines tip has later mono-unity/etc opts only |
| **VCA decay skip at equal sustain** | `e7b18d75` | `lane/optimizer-zl-coefcache-cand-b-lfo-20261106` (**branch missing**) | recreate from commit msg + logs; tests were `vca_sustain_test.*` | **NO — SHA+branch missing** |
| ZL LUT mixer / original_sound / BREAK retrigger | `7933fc32`,`0c2ad5f3`,`85012436`,`ce1a5da8`,`8aa6ec66` | local `zl-*` / missing ZL branches | partial content later appears under `lane/build-25-all-opt-struct-20261106` | Local SHAs missing; **themes partly on all-opt-struct, not tip** |
| Undo equal-word scan + silent BREAK skip | `3f4dffa2`,`2e178630` | `lane/optimizer-idle-20261106` | engine.c patch saved; undo later on all-opt-struct | Local SHAs missing; **undo word-scan IS in all-opt-struct tree** |
| Chicago Ensemble / modulation hoist | `ab6a2dd7`,`4ec5e825` | missing ZL-C branch | `patches/01a1ac46-*.patch` | Local SHAs missing; **`step[]` already in all-opt-struct `chicago_ens.h`** |

### Highest-value recoveries (not already sitting on an existing GH lane tip)

1. USB cooperative reset (`lost-02-…`) — unique, behavior-changing, not found on latency/all-opt/tip.
2. PERF #6 st_put chunk copy (`lost-06-…`) — unique vs storage tip (which only has LZ max-match).
3. Bus-FX call invariants (`lost-03-…`) — unique vs engines tip.
4. VCA sustain decay skip — unique; ZL-B branch never published.

## On GitHub but NOT in tip `a06e20189` (rebase fodder, not machine-lost)

| Branch | Tip SHA | vs `a06e20189` |
|---|---|---|
| `lane/optimizer-909-struct-20261106` | `df4d3f0c5cfd` | diverged ahead=236 |
| `lane/optimizer-fp-20261106` | `df4d3f0c5cfd` | diverged ahead=236 |
| `lane/optimizer-storage-20261106` | `c67eb8347bbb` | diverged ahead=230 |
| `lane/optimizer-engines-20261106` | `758c8e0d3e0b` | diverged ahead=234 |
| `lane/optimizer-latency-20261106` | `8081c00d2552` | diverged ahead=231 |
| `lane/optimizer-idle-20261106` | `b186be6c8494` | diverged ahead=232 |
| `lane/optimizer-idle-presets-20261106` | `ccb368f0d5b6` | diverged ahead=233 |
| `lane/optimizer-909-20261106` | `263a055879f4` | diverged ahead=236 |
| `lane/dev-m7a-notrun-exit-20261105` | `3c8b5d67e8da` | diverged ahead=197 |
| `lane/build-25-m1-port-20261106` | `b2b929f46f1b` | diverged ahead=227 |
| `lane/build-25-all-opt-struct-20261106` | `32802577dbf1` | diverged ahead=279 |
| `lane/dev-roadmap-evidence-strict-20261105` | `ea3fa9d0fd69` | diverged ahead=199 |
| `lane/dev-M4-sound-reset-data-v2-20261105` | `dd51bd3791b2` | diverged ahead=178 |
| `wip/chicago-opt` | `dd735533eace` | diverged ahead=1 |

`wip/one-build` @ `a06e20189` = tip (identical).

### Missing branches named in logs (never published)

- `lane/optimizer-909-cand-smp-20261106`
- `lane/optimizer-zl-coefcache-cand-a-cache-20261106`
- `lane/optimizer-zl-coefcache-cand-b-lfo-20261106`
- `lane/optimizer-zl-coefcache-cand-c-hoist-20261106`

(Note: some cand-A commits `527059a5`/`eb93c3ba`/`b5be0b83` *do* exist on GitHub as ancestors of all-opt-struct, just not under those branch names.)

## Unfinished / aborted threads

| Date (VLAD) | Sid | State | Goal |
|---|---|---|---|
| 2026-11-05 23:18 VLAD | `01a1abdc-568c` | unfinished | TASK (analysis only, no network needed, do not run git push, do not touch anything outside /workspac |
| 2026-11-05 23:20 VLAD | `01a1abde-8c29` | unfinished | You are an independent, advisory, READ-ONLY QA reviewer. Never push, commit to any remote, open PRs, |
| 2026-11-06 00:36 VLAD | `01a1ac23-8bb9` | aborted | # AGENTS.md instructions for /workspace/dnbox/ui/repo |
| 2026-11-06 01:47 VLAD | `01a1ac65-097c` | unfinished | # AGENTS.md instructions for /workspace/opt-909/repo |
| 2026-11-06 01:47 VLAD | `01a1ac65-087f` | unfinished | # AGENTS.md instructions for /workspace/opt-909/repo |
| 2026-11-06 01:47 VLAD | `01a1ac65-096e` | unfinished | # AGENTS.md instructions for /workspace/opt-909/repo |
| 2026-11-06 01:47 VLAD | `01a1ac65-0975` | unfinished | # AGENTS.md instructions for /workspace/opt-909/repo |
| 2026-11-06 01:48 VLAD | `01a1ac65-7500` | unfinished | # AGENTS.md instructions for /workspace/opt-909-smp/repo |

Several unfinished sessions are **909 optimizer** worktrees (`/workspace/opt-909*`) that never finished; related commits later appear on `lane/optimizer-909-*` under different SHAs.

## Patches directory

Saved under `/workspace/codex-import/patches/`:

- Session apply_patch artifacts: `01a1….patch` (analysis binders, reports, some firmware hunks).
- **Lost recoveries:** `lost-02-usb-retry-cooperative-reset.patch`, `lost-03-busfx-*`, `lost-06-project-stream-*`.

## Full thread table

| Date (VLAD) | Sid | State | Kind | Lane | Goal | Patches | Files | On GitHub? |
|---|---|---|---|---|---|---|---|---|
| 2026-11-05 23:00 VLAD | `01a1abcb-fc86` | finished | other | `—` | (no user prompt) | 0 | — | no |
| 2026-11-05 23:11 VLAD | `01a1abd6-5fdc` | finished | git-verify | `—` | In /workspace/pushc-dryrun/nightly-gh-pages (git clone of gh-pages; base 3c427179774016fc2 | 0 | — | no |
| 2026-11-05 23:11 VLAD | `01a1abd6-6c6d` | finished-but-blocked | analysis/QA | `lane/dev-M4-sound-reset-data-20261105` | You are an independent, advisory, READ-ONLY QA reviewer. Never push, commit to any remote, | 0 | — | yes @9427a68047af (NOT in tip, diverged) |
| 2026-11-05 23:15 VLAD | `01a1abd9-cb2a` | finished-but-blocked | analysis/QA | `—` | Write ONE file only: /workspace/docs-audit/lightweight/v2.1/_codex/RULES.md | 0 | — | no |
| 2026-11-05 23:17 VLAD | `01a1abdb-6375` | finished-but-blocked | analysis/QA | `—` | READ-ONLY. Do not push or modify git refs. In this repo, P=3c427179774016fc29d7baf7748e635 | 0 | — | no |
| 2026-11-05 23:18 VLAD | `01a1abdc-568c` | unfinished | analysis/QA | `lane/dev-M4-sound-reset-data-v2-20261105` | TASK (analysis only, no network needed, do not run git push, do not touch anything outside | 0 | — | yes @dd51bd3791b2 (NOT in tip, diverged) |
| 2026-11-05 23:20 VLAD | `01a1abde-8c29` | unfinished | analysis/QA | `—` | You are an independent, advisory, READ-ONLY QA reviewer. Never push, commit to any remote, | 0 | — | no |
| 2026-11-05 23:20 VLAD | `01a1abde-a766` | finished-but-blocked | analysis/QA | `lane/dev-M4-sound-reset-data-v2-20261105` | TASK (analysis only, no network needed, do not run git push, do not touch anything outside | 4 | bind.py | yes @dd51bd3791b2 (NOT in tip, diverged) |
| 2026-11-05 23:23 VLAD | `01a1abe1-3985` | finished | analysis/QA | `—` | Read-only analysis in /tmp of v.json (new versions.json), old.json (previous), i.html (ins | 0 | — | no |
| 2026-11-05 23:25 VLAD | `01a1abe2-6e58` | finished | analysis/QA | `—` | In this git repo, for commit 2501eef2316a9615c14bef46d5a7708352f36eef report: tree SHA, pa | 0 | — | no |
| 2026-11-05 23:35 VLAD | `01a1abeb-eb78` | finished-but-blocked | analysis/QA | `lane/dev-m7a-final-20261105` | TASK (analysis only, no network, no git push, no new test runs, no builds; touch nothing o | 7 | pass2.py | yes @ce2781295155 (NOT in tip, diverged) |
| 2026-11-05 23:49 VLAD | `01a1abf8-b588` | finished | analysis/QA | `lane/cache-retention-grok-20261105` | TASK (read-only analysis; no network, no builds, no suite runs, no git; write only under / | 2 | FEAT-REPORT.md | no |
| 2026-11-05 23:49 VLAD | `01a1abf8-b586` | finished | analysis/QA | `lane/dev-m7a-final-20261105` | TASK (read-only code review; no network, no builds, no suite runs, no git; write only unde | 0 | — | yes @ce2781295155 (NOT in tip, diverged) |
| 2026-11-06 00:03 VLAD | `01a1ac05-de98` | finished | analysis/QA | `wip/one-build` | TASK (read-only code review; no network, no builds, no suite runs, no git; write only unde | 1 | patch-dry-run.json | yes (in tip) |
| 2026-11-06 00:19 VLAD | `01a1ac13-c88f` | finished | analysis/QA | `lane/mb-smc-peek-734` | TASK (read-only code review; no network, no builds, no suite runs, no git; write only unde | 2 | dump-presets.py, IDLE-REPORT.md | no |
| 2026-11-06 00:19 VLAD | `01a1ac13-c88f` | finished | scratch-report | `wip/one-build` | TASK (no network, no git, no suite runs, no builds; write only under /workspace/dnbox/stri | 3 | roadmap_evidence_check.sh, roadmap_evidence_check_test.sh, SCHEMA.md | yes (in tip) |
| 2026-11-06 00:27 VLAD | `01a1ac1b-b848` | finished | analysis/QA | `—` | Write ONLY these two files (overwrite ok): | 0 | — | no |
| 2026-11-06 00:29 VLAD | `01a1ac1d-195f` | finished | scratch-report | `—` | TASK (no network, no git, no suite runs, no builds; write only under /workspace/dnbox/stri | 0 | — | no |
| 2026-11-06 00:33 VLAD | `01a1ac21-36ad` | finished | other | `—` | Write /workspace/docs-audit/lightweight/v3.1/_codex/RULES-firmware.md and RULES-nightly.md | 0 | — | no |
| 2026-11-06 00:34 VLAD | `01a1ac21-a891` | finished | optimizer-lane | `lane/main-ui-material-20261027` | # AGENTS.md instructions for /workspace/opt/wt-latency | 1 | D-latency.md | no |
| 2026-11-06 00:34 VLAD | `01a1ac21-a88a` | finished | optimizer-lane | `lane/build-25-m1-port-20261106` | # AGENTS.md instructions for /workspace/opt/wt-idle | 2 | engine.c, E-idle.md | yes @b2b929f46f1b (NOT in tip, diverged) |
| 2026-11-06 00:34 VLAD | `01a1ac21-a8c6` | finished | optimizer-lane | `lane/main-ui-material-20261027` | # AGENTS.md instructions for /workspace/opt/wt-storage | 0 | — | no |
| 2026-11-06 00:34 VLAD | `01a1ac21-a891` | finished | optimizer-lane | `lane/performance-font-renderer-cache-closure-20261025-r28` | # AGENTS.md instructions for /workspace/opt/wt-ui-lcd | 0 | — | no |
| 2026-11-06 00:34 VLAD | `01a1ac21-a8b7` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt/wt-engines | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 00:36 VLAD | `01a1ac23-6221` | finished | optimizer-lane | `lane/optimizer-idle-20261106` | # AGENTS.md instructions for /workspace/opt/wt-idle-presets | 0 | — | yes @b186be6c8494 (NOT in tip, diverged) |
| 2026-11-06 00:36 VLAD | `01a1ac23-8bb9` | aborted | git-verify | `lane/dev-M4-sound-reset-data-20261105` | # AGENTS.md instructions for /workspace/dnbox/ui/repo | 0 | — | yes @9427a68047af (NOT in tip, diverged) |
| 2026-11-06 00:38 VLAD | `01a1ac25-4ca9` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-engines | 0 | — | no |
| 2026-11-06 00:38 VLAD | `01a1ac25-4ca4` | finished | optimizer-lane | `lane/bugfinder-bf018-cq-bass-off-20261024` | # AGENTS.md instructions for /workspace/opt/wt-latency | 0 | — | no |
| 2026-11-06 00:38 VLAD | `01a1ac25-4ca4` | finished | optimizer-lane | `lane/build-25-m1-port-v2-20261106` | # AGENTS.md instructions for /workspace/opt/wt-storage | 1 | lz.c | yes @a18561f17977 (NOT in tip, diverged) |
| 2026-11-06 00:40 VLAD | `01a1ac27-3b2e` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-engines | 0 | — | no |
| 2026-11-06 00:41 VLAD | `01a1ac28-b0a5` | finished | optimizer-lane | `lane/dev-perf-nonui-20261105` | # AGENTS.md instructions for /workspace/opt/wt-engines | 0 | — | yes @ad073576fff8 (NOT in tip, diverged) |
| 2026-11-06 00:45 VLAD | `01a1ac2c-3295` | finished | other | `—` | In this bare git repo, run: git range-diff a0311156 b2b929f4 a0311156 fd3da791 ; and count | 0 | — | no |
| 2026-11-06 00:49 VLAD | `01a1ac2f-a54d` | finished | analysis/QA | `lane/bass-drum-ui-pages-20261026` | READ-ONLY. Repo clone at /tmp/git-verifier-strict/X0X37-for-Loopy-Pro. Target branch lane/ | 0 | — | no |
| 2026-11-06 00:49 VLAD | `01a1ac2f-c1d3` | finished | analysis/QA | `lane/browser-admission-repair-20261024` | Read-only task. Do not modify any git refs, do not push, do not delete. File /tmp/cands.ts | 0 | — | no |
| 2026-11-06 00:50 VLAD | `01a1ac30-ef97` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-idle | 0 | — | no |
| 2026-11-06 00:50 VLAD | `01a1ac30-ef98` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-latency | 0 | — | no |
| 2026-11-06 00:50 VLAD | `01a1ac30-ef97` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-engines | 0 | — | no |
| 2026-11-06 00:51 VLAD | `01a1ac31-7b54` | finished | optimizer-lane | `lane/build-25-m1-port-v2-20261106` | # AGENTS.md instructions for /workspace/opt/wt-latency | 3 | clip_latency_equivalence_test.sh, clip_latency_equivalence_test.c, D3-latency.md | yes @a18561f17977 (NOT in tip, diverged) |
| 2026-11-06 00:51 VLAD | `01a1ac31-7b66` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt/wt-engines | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 00:51 VLAD | `01a1ac31-7b52` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt/wt-idle | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 00:54 VLAD | `01a1ac34-4a4c` | finished | analysis/QA | `—` | READ-ONLY analysis. Working dir has files under /tmp/git-verifier-pushd/files/ and script  | 0 | — | no |
| 2026-11-06 01:02 VLAD | `01a1ac3b-2888` | finished | analysis/QA | `lane/browser-admission-repair-20261024` | Read /tmp/census-facts.json. This is a READ-ONLY post-deletion branch census after pruning | 0 | — | no |
| 2026-11-06 01:04 VLAD | `01a1ac3d-ac79` | finished | analysis/QA | `—` | Read-only verify Push D2 publication candidate. Files in cwd: parent-versions.json tip-ver | 0 | — | no |
| 2026-11-06 01:12 VLAD | `01a1ac44-dc51` | finished | optimizer-lane | `lane/bass-drum-ui-pages-20261026` | # AGENTS.md instructions for /workspace/opt-zl-lut/wb | 0 | — | no |
| 2026-11-06 01:12 VLAD | `01a1ac44-dc56` | finished | optimizer-lane | `lane/bass-drum-ui-pages-20261026` | # AGENTS.md instructions for /workspace/opt-zl-lut/wa | 0 | — | no |
| 2026-11-06 01:12 VLAD | `01a1ac44-dc48` | finished | optimizer-lane | `lane/dev-perf-nonui-20261105` | # AGENTS.md instructions for /workspace/opt-zl-lut/wc | 0 | — | yes @ad073576fff8 (NOT in tip, diverged) |
| 2026-11-06 01:14 VLAD | `01a1ac46-c42d` | finished | optimizer-lane | `lane/main-ui-material-20261027` | # AGENTS.md instructions for /workspace/opt-zl-coef/cand-b-lfo | 0 | — | no |
| 2026-11-06 01:14 VLAD | `01a1ac46-c438` | finished | optimizer-lane | `lane/build-25-m1-port-v2-20261106` | # AGENTS.md instructions for /workspace/opt-zl-coef/cand-c-hoist | 5 | chicago_ens.h, chicago_ens_hoist_test.c, run_chicago_ens_hoist.py | yes @a18561f17977 (NOT in tip, diverged) |
| 2026-11-06 01:14 VLAD | `01a1ac46-c428` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt-zl-coef/cand-a-cache | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 01:15 VLAD | `01a1ac47-6974` | finished | analysis/QA | `—` | Read-only analysis of local files under /tmp/p55d2. Do not network. Report PASS/FAIL lines | 0 | — | no |
| 2026-11-06 01:20 VLAD | `01a1ac4c-0898` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt-zl-silence/wt-voice | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 01:20 VLAD | `01a1ac4c-0886` | finished | optimizer-lane | `wip/one-build` | # AGENTS.md instructions for /workspace/opt-zl-silence/repo | 1 | codex-review.md | yes (in tip) |
| 2026-11-06 01:20 VLAD | `01a1ac4c-0888` | finished | optimizer-lane | `wip/mixer-ux` | # AGENTS.md instructions for /workspace/opt-zl-silence/wt-fxbus | 0 | — | no |
| 2026-11-06 01:21 VLAD | `01a1ac4d-0feb` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt/wt-engines | 2 | fxbus.c, a3_fxbus_equivalence_test.c, codex-engines-result.md | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 01:30 VLAD | `01a1ac55-5d1c` | finished-but-blocked | optimizer-lane | `lane/build-25-m1-port-20261106` | # AGENTS.md instructions for /workspace/opt/wt-latency | 2 | smk_clip_arp_share.c, clip_latency_equivalence_test.c, clip_latency_equivalence_test.sh | yes @b2b929f46f1b (NOT in tip, diverged) |
| 2026-11-06 01:43 VLAD | `01a1ac60-d205` | finished | scratch-report | `wip/one-build` | TASK (no network, no git, no suite runs, no builds; write only under /workspace/dnbox/stri | 0 | — | yes (in tip) |
| 2026-11-06 01:47 VLAD | `01a1ac64-7d9e` | finished | optimizer-lane | `lane/main-ui-material-20261027` | # AGENTS.md instructions for /workspace/opt-808-metal | 0 | — | no |
| 2026-11-06 01:47 VLAD | `01a1ac64-7db4` | finished | optimizer-lane | `wip/chicago-opt` | # AGENTS.md instructions for /workspace/opt-808-mix | 2 | drum808.c | yes @dd735533eace (NOT in tip, diverged) |
| 2026-11-06 01:47 VLAD | `01a1ac64-7d87` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt-808-voices | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 01:47 VLAD | `01a1ac64-7da0` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt-808-skip | 1 | drum808.c | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 01:47 VLAD | `01a1ac65-097c` | unfinished | optimizer-lane | `lane/optimizer-909-20261106` | # AGENTS.md instructions for /workspace/opt-909/repo | 0 | — | yes @263a055879f4 (NOT in tip, diverged) |
| 2026-11-06 01:47 VLAD | `01a1ac65-087f` | unfinished | optimizer-lane | `lane/optimizer-909-20261106` | # AGENTS.md instructions for /workspace/opt-909/repo | 3 | run_drum909_optimizer.py, drum909_optimizer_test.c, drum909.c | yes @263a055879f4 (NOT in tip, diverged) |
| 2026-11-06 01:47 VLAD | `01a1ac65-096e` | unfinished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt-909/repo | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 01:47 VLAD | `01a1ac65-0975` | unfinished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt-909/repo | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 01:48 VLAD | `01a1ac65-74ee` | finished | optimizer-lane | `lane/main-ui-material-20261027` | # AGENTS.md instructions for /workspace/opt-909-emit/repo | 0 | — | no |
| 2026-11-06 01:48 VLAD | `01a1ac65-7500` | unfinished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt-909-smp/repo | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 01:48 VLAD | `01a1ac65-7501` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt-909-bd/repo | 6 | identity.c, run.py, drum909.c | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 01:48 VLAD | `01a1ac65-e0da` | finished | other | `wip/one-build` | TASK (no network, no git, no builds of firmware, no full suite; you may compile/run indivi | 1 | ui.c | yes (in tip) |
| 2026-11-06 01:48 VLAD | `01a1ac65-fc7d` | finished | other | `lane/docs-lightweight-rules-20261106` | In this git repo, report commit count and author emails for origin/main..origin/lane/docs- | 0 | — | yes @1025d9b67468 (NOT in tip, diverged) |
| 2026-11-06 01:59 VLAD | `01a1ac6f-933b` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt-808-r2a | 0 | — | no |
| 2026-11-06 01:59 VLAD | `01a1ac6f-9342` | finished | optimizer-lane | `wip/chicago-opt` | # AGENTS.md instructions for /workspace/opt-808-r2b | 0 | — | yes @dd735533eace (NOT in tip, diverged) |
| 2026-11-06 02:07 VLAD | `01a1ac77-54af` | finished | analysis/QA | `lane/dev-perf-nonui-20261105` | READ-ONLY. Local clone at /workspace/git-verifier/perfnonui/repo. Verify branch tip origin | 0 | — | yes @ad073576fff8 (NOT in tip, diverged) |
| 2026-11-06 02:29 VLAD | `01a1ac8a-dd16` | finished | analysis/QA | `wip/one-build` | READ-ONLY for all git repos except a LOCAL scratch worktree: never push, never post, never | 0 | — | yes (in tip) |
| 2026-11-06 02:29 VLAD | `01a1ac8a-dd1a` | finished | analysis/QA | `wip/one-build` | READ-ONLY for all git repos except a LOCAL scratch worktree: never push, never post, never | 0 | — | yes (in tip) |
| 2026-11-06 02:29 VLAD | `01a1ac8a-dd0d` | finished | analysis/QA | `lane/velocity` | READ-ONLY for all git repos except a LOCAL scratch worktree: never push, never post, never | 0 | — | no |
| 2026-11-06 02:32 VLAD | `01a1ac8d-b793` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-909s-tip | 0 | — | no |
| 2026-11-06 02:38 VLAD | `01a1ac93-d13d` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-909s-cache | 0 | — | no |
| 2026-11-06 02:38 VLAD | `01a1ac93-d13d` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-909s-silence | 0 | — | no |
| 2026-11-06 02:38 VLAD | `01a1ac93-d136` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-909s-tanhx | 0 | — | no |
| 2026-11-06 02:38 VLAD | `01a1ac93-d177` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt/wt-909s-emit | 0 | — | no |
| 2026-11-06 02:39 VLAD | `01a1ac94-30a3` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt/wt-909s-cache | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 02:41 VLAD | `01a1ac96-2857` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt/wt-909s-emit | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 02:41 VLAD | `01a1ac96-27da` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt/wt-909s-tanhx | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 02:41 VLAD | `01a1ac96-27d9` | finished-but-blocked | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt/wt-909s-silence | 2 | drum909.c | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 02:41 VLAD | `01a1ac96-27f7` | finished | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt/wt-909s-tconst | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 02:44 VLAD | `01a1ac99-5b18` | finished-but-blocked | optimizer-lane | `lane/opt-preserve-909-init-20261021` | # AGENTS.md instructions for /workspace/opt-808-struct | 0 | — | yes @048b0aefeb2c (NOT in tip, diverged) |
| 2026-11-06 02:45 VLAD | `01a1ac99-d8b5` | finished | analysis/QA | `lane/dev-m7a-final-20261105` | READ-ONLY verify local clone at /tmp/git-verifier-strict2/repo. | 0 | — | yes @ce2781295155 (NOT in tip, diverged) |
| 2026-11-06 02:49 VLAD | `01a1ac9d-5bf2` | finished | optimizer-lane | `—` | # AGENTS.md instructions for /workspace/opt-808-struct | 0 | — | no |
| 2026-11-06 02:55 VLAD | `01a1aca2-aa8a` | finished | other | `—` | In this git repo, report: git rev-list --count bdc4965e..e5b8205d, and files changed by gi | 0 | — | no |
| 2026-11-06 02:55 VLAD | `01a1aca2-e8dd` | finished-but-blocked | git-verify | `—` | # AGENTS.md instructions for /workspace/qa2/e5b8 | 0 | — | no |
| 2026-11-06 03:00 VLAD | `01a1aca7-310c` | finished | git-verify | `—` | # AGENTS.md instructions for /workspace/qa2/d010 | 0 | — | no |
| 2026-11-06 03:01 VLAD | `01a1aca8-aa40` | finished-but-blocked | other | `—` | Run exactly: sh /workspace/qa2/panic-run.sh and paste its full output verbatim. Change not | 0 | — | no |
| 2026-11-06 03:01 VLAD | `01a1aca8-d695` | finished | analysis/QA | `lane/dev-perf-nonui-on-25opt-20261106` | READ-ONLY structural verification. Use only local git. Compact PASS/FAIL per tip. | 0 | — | no |
| 2026-11-06 03:03 VLAD | `01a1acaa-5f66` | finished | analysis/QA | `lane/dev-panic-20261106` | # AGENTS.md instructions for /tmp/pv | 0 | — | yes @9f4e90da197a (NOT in tip, diverged) |

## Ready Codex prompts (recover on top of `a06e20189`)

Shared constraints for all prompts: checkout/create lane from `wip/one-build` @ `a06e20189`; build **Elite + Pro**; run **full host suite** with **0 fail 0 skip**; no firmware binaries committed; never touch `main`, `nightly`, or `.github`. Prefer bit-identical / differential tests. Author identity = repo default.

### Prompt 1 — USB cooperative reset (lost)

```
Repo gobbajogger/X0X37-for-Loopy-Pro. Start from wip/one-build @ a06e20189.
Create lane/optimizer-latency-usb-coop-20261106 from that tip.
Apply the cooperative USB retry patch that splits usb_retry into reset + delayed attach
(25 ms TIMER4 settle) so main-loop input runs while USB reset settles. Source of truth:
the unified diff formerly applied as local ab6b6ae7 / optimizer patch
02-usb-retry-cooperative-reset.patch (contents recovered in this import as
lost-02-usb-retry-cooperative-reset.patch — firmware/src/usb.c only).
This is BEHAVIOR-CHANGING: say so in the commit message. Preserve boot blocking usb_start,
1s retry admission, detach cancel, attach/SIE sequence. Do not edit ui.c.
Add/adjust host coverage if feasible; else NOT_RUN honestly.
Then build Elite+Pro and run the full host suite: 0 fail 0 skip. No binaries. No push to main/nightly/.github.
Push only the lane branch and report SHA, size/RAM deltas for SMK37 if measured.
```

### Prompt 2 — PERF #6 storage chunk copy + bus-FX invariants (lost)

```
Repo gobbajogger/X0X37-for-Loopy-Pro. Start from wip/one-build @ a06e20189.
Create lane/optimizer-lost-storage-busfx-20261106.
Two separate commits:
A) PERF #6: in firmware/src/app/project.c st_put, replace per-byte sink copies with bounded
   memcpy up to each existing object boundary. Preserve packed bytes, object boundaries,
   flush failures and flash verification. Restore differential test
   tests/host/project_stream_chunk_copy_test.py from the recovered import artifact
   (lost-06-project_stream_chunk_copy_test.py / lost-06-C-storage.md). Local SHA was be168fc8 — never pushed.
B) Bus-FX call invariants: apply lost-03-busfx-call-invariants.patch to firmware/src/dsp/busfx.h
   (hoist cutoff target + rhythm periods once per nonempty call; keep sample recurrence).
   Add tests/busfx_equivalence_test.py from lost-03-busfx_equivalence_test.py.
   Local SHAs f8a015ae / ea0d18b6 — never pushed. Do not confuse with later engines tip opts.
No ui.c. Build Elite+Pro, full host suite 0 fail 0 skip, no binaries, never main/nightly/.github.
Push lane only; report SHAs and whether size/RAM moved.
```

### Prompt 3 — Rebase stalled optimizer stack onto tip (on GH, not in tip)

```
Repo gobbajogger/X0X37-for-Loopy-Pro. Tip wip/one-build @ a06e20189.
For EACH lane below, create a fresh catch-up branch from a06e20189 and cherry-pick/replay
ONLY that lane's own last patches (not unrelated merges), resolving conflicts without
expanding scope. Build Elite+Pro; full host suite 0 fail 0 skip; no binaries;
never main/nightly/.github. Report size/RAM for SMK37.
Lanes (GH tips, all diverged from tip):
- lane/optimizer-909-struct-20261106 @ df4d3f0c5 (alias optimizer-fp same SHA)
- lane/optimizer-storage-20261106 @ c67eb8347
- lane/optimizer-engines-20261106 @ 758c8e0d3e0b
- lane/optimizer-latency-20261106 @ 8081c00d
- lane/optimizer-idle-20261106 @ b186be6c
- lane/optimizer-idle-presets-20261106 @ ccb368f0d5b6
- lane/build-25-all-opt-struct-20261106 @ 32802577d  (largest merge stack; prefer as integration donor)
- lane/dev-m7a-notrun-exit-20261105 @ 3c8b5d67e
- lane/build-25-m1-port-20261106 @ b2b929f46
- lane/dev-roadmap-evidence-strict-20261105 @ ea3fa9d0f
- lane/dev-M4-sound-reset-data-v2-20261105 @ dd51bd379
Also recreate missing ZL-B commit e7b18d75 (Skip zero VCA decay arithmetic at equal sustain)
on top of tip if not already present after the all-opt replay; keep original (q+1)-1 rounding.
```

---
Generated from `/workspace/codex-import` session extract. Do not push from this report.
