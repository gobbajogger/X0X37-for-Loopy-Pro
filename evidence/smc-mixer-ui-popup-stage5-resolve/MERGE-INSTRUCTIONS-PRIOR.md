# Integrator merge instructions — SMC mixer UI popup (PR #13)

## Goal

Land the Stage-4–validated peek/hold/restore lane on `main` while preserving
history of validated tip `828298d9db1a403919e7e4c8176db83512281bb2`.

**Merge vehicle:** [PR #13](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/13)
(`cursor/smc-mixer-ui-popup-stage2-repair-5686` → `main`).

Do **not** rebase or force-push that tip. Prefer a merge commit (merge `main`
into the PR head, or merge the PR into `main` with conflict resolution).

## Current state (2026-10-22)

| Ref | SHA |
| --- | --- |
| Validated tip (PR #13 head) | `828298d9db1a403919e7e4c8176db83512281bb2` |
| `main` | `73d25b6d431aa5eaabaab54a0d640df2526fead4` |
| Merge-base | `e00741e2c1e8a018e7a7229dfd279e26aec34126` |

`main` advanced with **#9** (“Merge safe unmerged SMC/project work and test it”)
after the popup lane branched. GitHub reports PR #13 **CONFLICTING** / **DIRTY**.

## Conflict files

1. `BEHAVIOR.md` — both sides added an `SMC-05` rule.
2. `firmware/src/app/smc_mixer.c` — main: drum solo/SEQ/link/project; repair: peek/hold.
3. `tests/run_tests.sh` — main: new project/SMC tests; repair: `smc-ui-popup`.

`docs/SMC_MIXER.md` and most host tests auto-merge; keep the peek paragraph from
the repair side.

## Recommended resolution

### 1. Update PR head from main (history-preserving)

```bash
git fetch origin main cursor/smc-mixer-ui-popup-stage2-repair-5686
git checkout cursor/smc-mixer-ui-popup-stage2-repair-5686
git merge origin/main   # resolve conflicts; do not rebase 828298d9 away
```

### 2. `BEHAVIOR.md`

Keep **both** behaviors with distinct numbers (main already shipped drum rules):

- Keep main’s drum Record/Solo as **SMC-05** and pads as **SMC-06**.
- Add the popup rule from the repair tip as **SMC-07** (same text as repair
  `SMC-05` at `828298d9`, renumbered).

Alternatively, if the owner prefers the audit’s “SMC-05 = popup” label, renumber
main’s drum rules to SMC-06/SMC-07 and keep popup as SMC-05 — pick one scheme
and update any cross-refs.

### 3. `firmware/src/app/smc_mixer.c` (critical)

Start from **`origin/main`’s** `smc_mixer.c` (solo/link/project/drum SEQ).

Port the **validated repair** peek implementation from `828298d9` (not the
pre-repair `ad4f311` form):

- Table-driven strip → page map; fixed MIX MASTER page (`1+NKIT`).
- Peek state in a **byte buffer** (IPA-SRA-safe).
- `smc_peek_begin` / `smc_peek_tick` / restore via assign + `build_pages()` only
  (never `set_view` for peek/restore).
- Hold ≈ 1200 ms; refresh on continued same-peek moves.
- Guards: no peek in clip, help, overlays, `perf_testing()`; drum MIX skip.
- Tick from `smc_frame`; overlay/help cancel in `smc_poll` as on the repair tip.

Wire peek hooks into the main-mixer fader/knob / `smc_show` paths without
undoing main’s drum solo, link generation, or project-reset behavior.

**This changes checked firmware sources.** After the merge tip exists:

1. Re-run focused SMC host suite (include `smc_ui_popup` + main’s new SMC tests).
2. Re-run Stage-4 formal Elite/Pro DEV validate on the **merge tip**.
3. Do **not** flash Stage-4 packages from `828298d9` as if they were the merge tip.

### 4. `tests/run_tests.sh`

Take **main’s** registrations, and add:

```text
smc-ui-popup
```

in the G-class list and as `run smc-ui-popup sh tests/host/smc_ui_popup_test.sh`
next to the other `smc-*` runs (same placement as on `828298d9`).

### 5. Finish PR #13 for human merge

```bash
git push origin cursor/smc-mixer-ui-popup-stage2-repair-5686
# Mark ready for review (human/integrator; Stage-5 cloud cannot undraft):
gh pr ready 13
# Human merges via GitHub UI or authorized token — do not use unauthorized gh pr merge
```

Cloud Stage-5 cannot: ManagePullRequest (missing), `gh pr` write (403), or
`gh pr merge` (forbidden).

## What Stage 5 did / did not do

| Did | Did not |
| --- | --- |
| Confirm validated tip and PR #13 | Merge to `main` |
| Record conflicts and exact steps | Rewrite `828298d9` |
| Write Stage-6 **local** install handoff | Flash firmware / install |
| Preserve repair history as merge vehicle | Implement page-lock / multi-SMC |
| | Re-validate budgets on `828298d9` |

## Stage-6 note

Device install of the **already validated** Elite/Pro packages may proceed from
a **local** Stage-6 thread against tip `828298d9` (see `stage6-prompt.txt`) even
while PR #13 remains conflicted — that checks peek/hold/restore on device.
Landing on `main` still requires the conflict resolution + re-validate path above
before the merged tree’s packages are trusted.
