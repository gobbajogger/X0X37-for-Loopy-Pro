# One-build plan: installable build with every built feature (2026-10-30, VLAT UTC+10)

Source: docs/FEATURE-REGISTRY.md (#723) on wip/one-build, open PRs at 20:45 VLAT.
CI: almost all PR checks fail or have no run; the red host-test base blocks every merge.
Effort: S < 1 h, M 1–3 h, L > 3 h of bot work.

## 1. Built features and status
| Feature | Status | Source | CI / blocker |
|---|---|---|---|
| Paged SMC momentary FX, SMC SELECT, Chicago FX inserts, 20 BPM delay fix, sample backup, Night UI, SMC credits | in wip/one-build | – | – |
| Free Play pads with Miami rate off | open PR | #715 | needs green base |
| MIDI queue order / stuck notes | open PR | #705 | check overlap with #677 |
| Chicago FX state preserved | PR on nightly | #677 | take FX-state part only onto one-build |
| SMC one-mixer connection | open PR | #700 | full suite not posted |
| SMC first boot + BT roles | open PR | #709 | merge after #700 |
| SMC dual-peer reconnect | PR on nightly | #687 | Roman: superseded by #700? |
| SMC locked mixer pages | open PR | #717 | CI FAILURE, re-QA |
| Mixer FX page full (ID 41) | open PR | #725 | 373/373 host PASS claimed |
| Engines UI + faster Chicago/Miami | open PR | #712 | conflicts #691, #692 |
| Miami speed-up | open PR | #691 | 15 .f32 goldens missing |
| Chicago optimisation | open PR (nightly) | #692 | conflicts #712 |
| Clip-mode held < > | open PR | #719 | fix pushed, re-QA |
| DJ filter centred | open PR | #696 | CI FAILURE, NEEDS CHANGES |
| Remaining features (25 IDs) | open PR | #694 | CI FAILURE, cherry-pick items only |
| BREAK slices/pitch/stretch | open PR | #689 | needs rebase check |
| SELECT page wrap | PR on nightly | #693 | cherry-pick 3 commits |
| Composition stack | branch only (partial) | lane/main-ui-material-20261027 (#650) | old base |
| SMC popup UI stages 1-6 | branch only (old base) | cursor/smc-mixer-ui-popup-stage6-blocked-8e03 | old base |
| Web editor (partial) | open PR | #685 | 32 behind, rebase |
| Installer + website | PR on nightly | #690 | separate merge, not in firmware |

Not built at all (planned): Elite/Pro native packages SDK 1.2.1 (112), updated NIGHTLY (111), project export/import + format compat (50, 54), recovery bootloader (73), editor iPhone UI/note groups/scale-snap/device protocol/TouchOSC (97, 98, 101, 104, 105), community library/vote inbox/contribution audit (119, 120, 122).

## 2. Critical path
1. **Green base** (blocks all): #721 private headers (fix docs note, S), then #724 host-test repair folding #710 and #704 (without its workflow file) and #708 (strip dsp/bass303.c) — L. Owner: Build and Test Runner. Merge: Merge Integrator.
2. **Low-risk fixes**: #715 (S), #705 (S), FX-state part of #677 (S, Main Builder makes a one-build PR), #719 (S after re-QA, Second Builder).
3. **SMC chain**: #700 (M, full suite) → #709 (S, rebase) → #717 (M, fix CI + re-QA) → #725 (M, rebase on #717). Owner: Second Builder; Roman decides #687.
4. **Engines**: #712 (M) → #691 rebased + goldens (M) → #692 rebased onto one-build (M). Owner: Firmware Main Builder.
5. **UI**: #696 glyph/test delta (M), #694 selected items (L), #693 three commits (S), #689 BREAK (M). Owner: Firmware Main Builder (UI), Merge Integrator (#693 cherry-pick).
6. **Docs** any time, no firmware CI needed: #718, #720, #722, #726, #727 (rebase, conflicting), #716, #706, #699, #703, this plan.
7. **Package**: Release Packager builds from the wip/one-build head after step 5: firmware image, full host suite log, HW checklist (#718), feature list from the registry, install notes. Only then tell Roman it is ready (owner rule: no partial installs).

Out of scope for this build: #685, #690 (web), composition stack and popup UI (old base, need a port; plan as new work).

## 3. Ordered merge queue
#721 → #724 (+#710, #704, #708) → #715 → #705 → #677-part → #719 → #700 → #709 → #717 → #725 → #712 → #691 → #692 → #693-part → #696-part → #694-part → #689 → package.
Rebase after each merge: every later PR on the list.

## 4. ETA guess
Green base 4–8 h. Steps 2–5 with serial merges and re-test: 1–2 days. Installable build: about 2 days (1 Nov 2026 VLAT) if CI is green by tomorrow morning; add 1 day per extra base failure round.
