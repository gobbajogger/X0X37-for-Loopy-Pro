# Orchestrator plan: X0X37 full-cycle loop

> HISTORICAL RECORD - NOT AN ACTIVE WORKFLOW. This dated plan and its
> thread-launch instructions are retained as provenance only. Use the current
> [project workflow](../rules/workflow.md) and current request/owner checkpoint.
> It grants no new execution, retry, integration, publication or device authority.

You are the orchestrator. You do not implement, test, merge, or publish the firmware yourself. You start one owner-visible Cloud Agent per stage, record its URL, and start the next stage only after that agent pushes its handoff. The owner must be able to open every stage thread.

Repository: https://github.com/gobbajogger/X0X37-for-Loopy-Pro
Procedure already in the repo: `AGENTS.md` and `docs/rules/workflow.md` on `cursor/full-cycle-stage-workflow-d6ff`.
Nightly installer, not production: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/
Nightly browser repo: `gobbajogger/x0x37-for-Loopy-Pro_nightly`

## Where the loop is now

Stage 1 finished. Stage 2 has not started.

- Audit branch: `cursor/f02-loader-stage1-e755`
- Audit tip: `5293bcd8e9a4152c5dcdbd515390ee3a8e60f945`
- Audit: `evidence/f02-loader-stage1/AUDIT.md`
- Stage-2 prompt: `evidence/f02-loader-stage1/SUCCESSOR-STAGE2.txt`
- Audited tree: `e23170227ca4ae4b6cc2700692f4d650fa1b506b`
- Stage-1 run `bc-ad31708d-5548-5785-bb62-75a54918e755` is internal. The owner cannot open it. Do not resume it and do not count it as the successor.

Selected defect: **F02**. After the installer sends the selected keyboard into its update loader, the next search can attach to a different keyboard and write the package there.

Approved repair, and nothing else:

- Change `tools/fm1_install.py` and `web/fm1ota.js`.
- Apply the same rule where `web/x0x_breaks.js` `installKeepingSlots`, `web/x0x_installer.html`, and `web/index_pkg.html` continue a transition that has already started.
- Snapshot loader ports before the transition. Accept exactly one newly appeared loader. Otherwise fail closed and do not write.
- Use that same rule for the post-write wait, including the slot-restore wait.
- Leave the initial "first running keyboard" scan unchanged.
- Add a regression for the native `wait_for` path and the web `waitFor` path. The test must not open real MIDI and must not write a device.
- Do not implement F03, BF-001, BF-002, BF-003, or the withheld asset cache.
- Do not change firmware, the controller image, or package bytes for this repair.
- Development configuration, if a firmware build is ever required: `X0X_DEV_PADCAL=1`.

## First action

Create exactly one owner-visible stage-2 agent. `startingRef` is `cursor/f02-loader-stage1-e755`. The prompt is the contents of `evidence/f02-loader-stage1/SUCCESSOR-STAGE2.txt` on that tip, plus this instruction: when the stage is done, push the stage-3 prompt and return the branch, tip, and handoff path to the orchestrator. Do not ask that agent to create the next thread. You create it.

After the create call returns, read the agent back. Record it only if its source is one the owner can open (`web`, `mobile`, `desktop`, `api`, `slack`, `linear`, `scm`, or `automations`) and the URL is `https://cursor.com/agents/<id>`. Push that id and URL into the stage-1 handoff. An internal Task subagent does not count. If the create call fails, say so and keep this stage. Do not start a second stage-2 agent while the first one is running.

## The loop

Run one stage at a time. Do not create the later stages in advance. Do not resume a stage agent into the next stage.

1. **Audit.** Inspect the named SHA. Record defects, evidence, scope, the approved fix, and missing decisions. No production edits. A recorded defect goes to stage 2. No recorded defect goes back to a new stage 1. The loop does not end.
2. **Implement.** Approved fixes only, on a new `cursor/<slug>-stage2-<suffix>` branch from the pushed audit tip. Scoped checks and a commit. Development configuration only.
3. **Test.** Fresh agent on the implement tip. Separate internal subagents may run the tests and investigate a failure. A source repair goes back to a new stage 2, then a new stage 3. The test agent does not patch the failure forward.
4. **Validate.** Elite and Pro development packages, exact identities, budgets, and logs. A host pass is not a package pass. Pool free at least 8192 bytes. Overflow or a changed checked input fails the stage. No full release gate unless the current request explicitly asks for a release build.
5. **Merge.** One integrator. Fast-forward firmware `nightly` to the validated SHA when the remote accepts a normal update. If the remote refuses, record the refusal and do not force-push. Stage 7 still uses that validated SHA. Save the local install prompt. Do not update production `main`.
6. **Install.** Local machine with the device only. You do not start a cloud installer. A cloud attempt writes `INSTALL BLOCKED` and does not flash. This does not hold up publish or the next cycle.
7. **Publish nightly.** Only a candidate that passed every prehardware gate. Label `DEV / HARDWARE TEST — DEVICE NOT TESTED`. `hardware_tested` stays false. Link immutable hosted package bytes. Fetch them again and require size and SHA-256 to match. Fast-forward `gh-pages` in the nightly browser repo through its publisher route, then check the live page. Leave `.fwsc` and `ctl.img` out of Git. Leave production `main` and production GitHub Pages unchanged. A failed gate stays out of the flashable picker and goes back to stage 2. A valid candidate whose page update fails goes to one new stage 7, not to stage 1. After the live page matches, start the next stage 1 on that same SHA.

Next defect order for each new audit: the oldest deferred defect from any cycle that is not already fixed in that SHA, then a recorded failing check that is not already fixed in that SHA, then the oldest open audit or bugfinder finding not already contained in that SHA. Do not ask the owner which defect to take.

An unchanged SHA with no new defect still starts the next audit. That is the loop. The only reason not to start the next stage is that its owner-visible agent is already running, or the create call failed.

## How you start a thread the owner can open

Use `POST https://api.cursor.com/v1/agents` with the owner's user or service-account API key (`CURSOR_API_KEY`). Send:

- repository `https://github.com/gobbajogger/X0X37-for-Loopy-Pro`
- `startingRef` set to the pushed stage branch
- `prompt.text` set to that stage's successor file, plus the return instruction above

The pod identity token is not an API key. Do not send it. A Task tool subagent, local or cloud, is internal to the parent run. The owner cannot open it. Do not use one as the stage thread.

Before you create, read the remote handoff and the owner's agents. If this handoff already has an owner-visible successor URL, keep it. Do not start another.

When the agent returns its handoff, confirm the prompt commit is on the remote before you start the next stage. A saved prompt with no owner-visible URL is not a completed handoff.

## What you do not do

- Do not implement F02 in this orchestrator thread.
- Do not flash a device.
- Do not publish a package that failed a gate.
- Do not treat a Git push or a nightly page as a production release.
- Do not merge production `main` or production Pages. Those need the owner's device pass on the same bytes and a separate production approval.
- Do not wait for the owner between stages.
- Do not create stages 3 through 7 until stage 2 has pushed its handoff.
