# Full-cycle dev chain

Use this procedure for an authorized firmware or code change. Each stage is its own thread. Subagents do the work inside that thread. The owner of stages 1–5 and stage 7 starts exactly one new cloud thread, pushes that thread's id, and only then hands off. Stage 6 starts none.

The cloud process does not pause for the owner and does not end after one cycle. A cycle publishes a valid nightly build to the nightly installer page, <https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/>, then stage 7 starts the next cycle's audit. That page is the public preview in `gobbajogger/x0x37-for-Loopy-Pro_nightly`. It is not production `main` and not production GitHub Pages.

A docs-only edit does not start this chain. Leave the removed skill pack, ChatGPT project routing, and Codex `create_thread` instructions unrestored.

## One fresh thread per stage

Cloud threads run in order: stage 1, then 2, then 3, then 4, then 5, then 7. The current cloud stage creates the next cloud thread only after its own checks pass. Leave the later cloud stages uncreated until then. Stage 6 is the local install prompt saved by stage 5. It is not a cloud thread between stage 5 and stage 7, and stage 7 does not wait for it. The thread assigned to a stage does that stage. It does not create a second copy of itself.

Never pass `resume` to turn a stage agent into the next stage. A stage change needs a new thread.

| Stage | Starting state and goal | Done when |
| --- | --- | --- |
| 1. Audit | Inspect the named validated SHA and the intended behavior. Write defects, evidence, affected scope, proposed fixes, which fixes already have approval, and which decisions are still missing. Make no production edits. | The audit, scope, and approvals are recorded, and one stage-2 thread has started. The idle case records an unchanged SHA and starts no stage 2. |
| 2. Implement | Branch from the pushed audit tip as `cursor/<slug>-stage2-<suffix>`. Apply only the approved fixes. Use the development configuration `X0X_DEV_PADCAL=1`. Run the scoped checks and commit. | Checked commits for the approved fixes are on the remote branch, deferred items are recorded, and one stage-3 thread has started. |
| 3. Test | Start from the pushed implement tip. Give the test run and any failure investigation to separate subagents. | The relevant tests pass, and each failure has a cause and a disposition. Then one stage-4 thread has started. |
| 4. Validate | Build Elite and Pro development packages from the tested candidate. Record exact package and controller identities, budgets, checks, and logs. | Both development packages meet the budgets. A host pass is not a package pass. Then one stage-5 thread has started. |
| 5. Merge | One integrator fast-forwards firmware `nightly` to the validated tip when the remote accepts that normal update, and saves the local install prompt. | The handoff names that validated SHA. `nightly` matches it, or the refusal is recorded without a force-push. One stage-7 thread has started on that same SHA. |
| 6. Install | A local hardware thread installs the matching development artifacts and records installation and data-preservation checks. Stage 5 saves this prompt. Stage 6 starts no cloud thread. | A local install record exists for those artifact hashes. A cloud attempt with no device records `INSTALL BLOCKED` and leaves this stage pending. |
| 7. Publish nightly | Publish the valid development packages to the nightly installer page. Then start the next cycle. | The live page serves that candidate, labeled for hardware test, and the served package hashes match. One new stage-1 thread has started. The page is not a production release. |

Use the current run's required branch prefix and suffix. Stages 1–5 each get their own `cursor/<slug>-stageN-<suffix>` branch. The successor works on its own new branch. The sender does not merge the successor's branch back into its own. Stage 7 starts from the pushed stage-5 branch and publishes in the nightly repository. Stage 6 runs from the saved prompt on a local machine.

Stage 4 stays on development packages. A full release gate runs only when the current request explicitly asks for a release build. Production `main`, production Pages, and a public release still need the owner's device pass for that same installed build, plus a separate production approval, in a fresh thread, and only when that evidence exists. This chain does not perform that promotion.

Before installation, compare the merged inputs with the stage-4 evidence. A new commit id by itself leaves matching evidence valid. A change to checked source, configuration, tools, dependencies, or artifacts sends the work back to a new thread at the earliest affected stage. Repeat a completed merge only when another merge is actually required. That new merge goes through the affected validation and merge stages again.

Stage 5 is the only integrator. It updates firmware `nightly` to the validated SHA with a normal fast-forward. Recheck open agents and that branch before the update so two integrators do not compete. If the remote refuses the update, record the refusal and do not force-push. Stage 7 and the next audit still use that validated SHA, not an older `nightly` commit. Stage order does not add permission for production `main`, production Pages, or a new defect fix. Carry existing approvals forward.

Stages 1–5 and stage 7 each start one cloud successor. Stage 5's successor is stage 7, recorded in `SUCCESSOR-STAGE7.txt`. Stage 5 also saves the stage-6 local install prompt, including the package and controller hashes. That prompt is not a cloud thread. Stage 6 starts no successor. Stage 7's successor is the next cycle's stage 1, recorded in `SUCCESSOR-STAGE1.txt`. A later local install records the device result and does not publish the page, start a cycle, or push production `main`.

## Subagents inside the stage

Delegate the stage's real work to fresh subagents. Each helper is a new Task call with no `resume`. Use separate scopes: stage 3 uses one subagent for the test run and another for failure investigation. Other stages use a fresh subagent for the stage's checks or review.

A subagent supports the stage. It does not replace the stage thread, and it does not launch the next stage. Only the owner of stages 1–5 and stage 7 launches the successor.

## Successor handoff

A stage hands off only after its new thread is started and that start is on the remote. A pushed prompt without an agent id is not a handoff. The current owner still owns the work until the id commit is pushed.

When stage 1, 2, 3, 4, 5, or 7 passes its checks:

1. Write `evidence/<slug>-stageN/HANDOFF.md` and the successor prompt. Stages 1–4 write `SUCCESSOR-STAGE{N+1}.txt`. Stage 5 writes `SUCCESSOR-STAGE7.txt` and the stage-6 local install prompt. Stage 7, after the live page matches, writes `SUCCESSOR-STAGE1.txt` for the next cycle. A stage-7 thread that could not update the live page writes a fresh stage-7 prompt instead, and does not start stage 1. The prompt names the next stage, repository, validated SHA, branch, approved scope, evidence paths, completion condition, and this same rule. Stage 6 does not write a successor prompt and does not launch a thread.
2. Commit and push that prompt. The new thread can see only the remote tip. This push prepares the handoff. It does not complete it.
3. Read the remote handoff and the open cloud agents. If `HANDOFF.md` on that remote already records a successor agent id for this prompt, the thread is already started. Keep that id and do not start another. If no id is recorded, look for a running successor for this prompt: a passing stage 7 looks for a running stage-1 of the same validated SHA and the same defect; a stage 7 that could not update the live page looks for a running stage-7 retry of that same candidate. Persist a found id as in step 5.
4. If no id is recorded and no successor is running, start exactly one new thread with the Task tool. Use `environment` `cloud`, `cloud_base_branch` set to the pushed branch that contains the prompt and the validated SHA, `run_in_background` true, and no `resume`. Put the full prompt in the task, including that SHA. The next audit uses that SHA. It does not switch to a different `nightly` commit.
5. Write the returned agent id into `HANDOFF.md`, commit it, and push that commit. The handoff is complete when that id is on the remote. Then the current owner stops. It does not wait for the next stage to finish.

If the tool returns no agent id, the handoff is not complete. Push a note that the start is unstarted, keep ownership, and do not stop as if the next stage has begun. Do not claim it launched. Do not call the Cloud Agents HTTP API. Do not substitute a local helper for the next stage. Do not start a second thread while the first start is still uncertain. Confirm no successor is running, then retry the start once. A second empty result leaves the stage with its current owner.

A failed check is not a completed stage. Start one new thread at the earliest affected stage. An `INSTALL BLOCKED` cloud note is not a failed package check and is not an install pass. A source repair found in stage 3, a package or source failure found in stage 4, or a prehardware gate that fails in stage 7 returns to a new stage 2, then a new stage 3, then a new stage 4. The test thread does not patch the failure forward. Do not advance to merge, install, or the nightly page while checks are failed. Do not leave the failure without a successor.

Missing hardware blocks the device install record only. Do not skip a failed package check and publish anyway. A valid candidate still publishes when no device is attached.

## Valid nightly page

Stage 7 runs after stage 4 and stage 5. It does not wait for a physical install. Follow the nightly repo's `docs/nightly-first-development-gate.md` and `docs/qualified-nightly-candidate.md`.

A candidate is valid only when every prehardware gate passes for the exact source tree: both the Elite and the Pro development packages, model and header identity, package and controller CRC and SHA-256, source attribution, memory and flash budgets with at least 8192 bytes of pool free, host regressions, backup, readback, and rollback, installer and model negatives, independent review, and a lawful distribution of the package bytes. The page label is `DEV / HARDWARE TEST — DEVICE NOT TESTED`. `hardware_tested` stays false.

Publish by linking those immutable hosted package bytes from the nightly installer. Fetch the bytes again and require the size and SHA-256 to match before the page can offer them. Reconcile the existing qualified catalog. Fast-forward the nightly repo's `gh-pages` through its publisher route, then fetch the live installer and confirm the served catalog, installer, and package hashes.

Leave `.fwsc` packages and `ctl.img` out of both Git repositories. Leave proprietary SDK, toolchain, DSP references, samples, and secrets out of the nightly repo. Leave production `main` and production GitHub Pages unchanged.

A failed, skipped, or missing gate stays out of the flashable picker. Record that history, and start one stage-2 repair thread for that failure. Do not publish the candidate and do not start stage 1 for a new defect in parallel. If this thread cannot publish a candidate that already passed every gate, save the qualified handoff and start one fresh stage-7 retry of that same candidate under the successor rule. A successor id already on the remote, or a retry that is already running, blocks a second start. Do not start stage 1 from a page that did not update. A prepared folder is not a live page.

After the live page matches the candidate, start the next cycle as specified below. Do not treat the page as a production release.

## Next cycle

This section applies only after the live page matches the candidate. Stage 7 then starts exactly one fresh stage-1 thread, pushes that thread's id, and only then ends its own turn. It does not wait for a device install, an owner message, or a new request. The successor prompt names the validated SHA that was published, the page hashes, and this procedure. `cloud_base_branch` is the pushed branch that contains that SHA and the handoff. The audit uses that SHA even when firmware `nightly` is behind it.

The next defect is the oldest deferred defect from any cycle that is not already fixed in that SHA, then a recorded failing check that is not already fixed in that SHA, then the oldest open audit or bugfinder finding not already contained in that SHA. The new audit does not ask which defect to take. It does not resume stage 7. If a stage-1 successor id for that same SHA and the same defect is already on the remote, or that successor is already running, stage 7 keeps that id and does not start a second cycle. The handoff finishes only when the id is pushed.

When no such defect is recorded, the new audit inspects that same SHA and its evidence once. New evidence on that SHA continues into stage 2. An unchanged SHA with unchanged evidence is the only idle case: record it, and start no implement thread and no further audit. A failed page update is not this case. It starts a stage-7 retry of the same candidate when none is running. A failed package or prehardware gate returns through stage 2, stage 3, and stage 4 in new threads.

## Local install

Stage 6 runs on a machine with the device attached. The stage-5 owner saves the local prompt and does not spawn a cloud installer.

A cloud thread that is asked to install writes `INSTALL BLOCKED`, the package and controller hashes, and the local prompt, then stops. It does not flash the device. The stage-7 publication thread is the only cloud successor of stage 5.
