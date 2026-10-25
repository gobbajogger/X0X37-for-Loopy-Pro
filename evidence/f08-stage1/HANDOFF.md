# Workflow handoff to stage 2

Receiving stage: **2 Implement** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f08-bf004-stage1-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-1 owner: `bc-78e96d95-98c8-5fe4-b4f0-287bc2ad068d`.
Audited / validated source SHA: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`.
Content tree: `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`.
Git tree: `4dc57f74141fd1f01728a0001025b6e76065bf19`.
Audited firmware content tree: `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`.
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03), `1325f5d3` (BF-003), `6df0f81f` (FP-001) — not re-opened.
Audit record: `evidence/f08-stage1/AUDIT.md`.
Checks: `evidence/f08-stage1/CHECKS.md`.
Prior publish evidence: `evidence/f07-stage7/CHECKS.md`, `HANDOFF.md`, `live-verify.log`, `independent-qa-local-review.md`.
Independent claim review: internal Task `bc-5b578719-45a1-5aa8-9ae1-25087ce3609e` (12/12 PASS).
Stage-7 local review: Project store `internal/pr-local-reviews/pr-225.md` (PASS).
Package local review: Project store `internal/pr-local-reviews/pr-222.md` (PASS).
Deferred younger / excluded items: withheld asset cache, unmerged data-safety identity lanes, HTML initial-loader resume `find(IS_OTA)`, longer `SLOT_PROBE` budgets, delete Felucca-era `make_site.py` entirely, nightly-repo second qualification field, historical F06 Stage-7 “Installer FP” wording rewrite (`evidence/f07-stage2/DEFERRED.md`); younger BF-005/BF-006/BF-007.
Successor prompt: `evidence/f08-stage1/SUCCESSOR-STAGE2.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 2 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required after the prompt is pushed.

Selected defect: **BF-004**. Stale Note Off from a previous USB/Bluetooth source releases the current owner’s 303 / BREAK hold because `midi_in` does not check `src_held` ownership on Note Off (`firmware/src/app/engine.c`, blob `555480d9…`). **Approved repair: ownership-gated Note Off / BREAK off + host regressions.** Do not reopen BF-001–BF-003 or FP-001.

## Scope

Stage 2 only. Apply the approved BF-004 fix. Do not implement other deferred or younger bugfinder items. Do not merge production `main`, publish production Pages, or flash a device. Use `X0X_DEV_PADCAL=1` when a firmware build is required.

When checked commits are on the remote, write the stage-3 successor prompt, push it, and start exactly one owner-visible stage-3 Cloud Agent under the same handoff rule (coordinator CreateAgent if no API key). An internal Task subagent does not complete the handoff.

## Live nightly page (unchanged by this audit)

- Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
- `latest`: `nightly-6df0f81f-dev`
- Elite / Pro bundled: `8ba23a89…` / `d9093211…`
- Source FP / served HTML: `8caf8724…` / `d0866927…` (distinct)
- Pages tip: `d6a237c43cb6830d8cf0cb51e567181821630a28`
