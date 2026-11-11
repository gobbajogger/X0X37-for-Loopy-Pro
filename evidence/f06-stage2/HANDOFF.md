# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f06-bf003-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md` on that branch.
Parent / stage-2 owner: `bc-764bccc6-54f9-58a3-bf70-6f95773c3460`.
Audited / validated source SHA: `ab90f47ccad0f213114d6a98c1bc6179be2ebb35`.
Content tree: `0ee03318a0b64a1e535b36fad284558529ff7e3f95ff069efed86bbcf308518c`.
Git tree: `50c381c528cae9692deff4a712071f7dc6acb020`.
Repair commits already in tip: `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03) — not re-opened.
BF-003 implement commit: `1325f5d34340ea877e65ab86f3663f5dea052c0d` (`Retire BF-003 stale editor.html test and site-builder links`).
Implement evidence: `evidence/f06-stage2/CHECKS.md`, `evidence/f06-stage2/DEFERRED.md`.
Audit record: `evidence/f06-stage1/AUDIT.md`.
Stage-2 PR: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/207
Successor prompt: `evidence/f06-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete** until an owner-visible Stage 3 id is recorded on this remote tip. Pod has no `CURSOR_API_KEY`; coordinator CreateAgent is required.

## Repair summary

BF-003 only. Retired stale Felucca editor references: stripped editor-dependent sections from `web/test_web.mjs` (kept `fm1pkg`/`fm1ota` FakeFM1 checks), updated `web/make_site.py` to installer-only (no `editor.html` copy; removes stale `webapp/editor`), adjusted `tools/sampleio.py` / `web/EDITOR_PROTOCOL.md` comments. Did not restore `editor.html` / `editor.c`. Local proof: `node web/test_web.mjs` exit 0; make_site dry run exit 0. Deferred items in `evidence/f06-stage2/DEFERRED.md`.

## Scope for stage 3

Run the BF-003 host/node checks on the implement tip. Separate subagents for the test run and for failure investigation. Do not implement deferred items. Do not restore the editor. Do not merge production `main`, publish production Pages, or flash a device. Do not change firmware, controller, or package bytes.

When checks pass, write the stage-4 successor prompt, push it, and start exactly one owner-visible stage-4 Cloud Agent under the same handoff rule. An internal Task subagent does not complete the handoff.
