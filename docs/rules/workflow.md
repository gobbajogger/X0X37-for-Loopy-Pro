# Full-cycle dev chain

Use this procedure for an authorized firmware or code change. Each stage is its own thread. Subagents do the work inside that thread. The owner of stages 1–5 starts exactly one cloud successor, records its id, and stops. Stage 6 and stage 7 start none.

The chain ends by publishing a valid nightly build on the nightly installer page: <https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/>. That page is the public preview in `gobbajogger/x0x37-for-Loopy-Pro_nightly`. It is not production `main` and not production GitHub Pages.

A docs-only edit does not start this chain. Leave the removed skill pack, ChatGPT project routing, and Codex `create_thread` instructions unrestored.

## One fresh thread per stage

Cloud threads run in order: stage 1, then 2, then 3, then 4, then 5, then 7. The current cloud stage creates the next cloud thread only after its own checks pass. Leave the later cloud stages uncreated until then. Stage 6 is the local install prompt saved by stage 5. It is not a cloud thread between stage 5 and stage 7, and stage 7 does not wait for it. The thread assigned to a stage does that stage. It does not create a second copy of itself.

Never pass `resume` to turn a stage agent into the next stage. A stage change needs a new thread.

| Stage | Starting state and goal | Done when |
| --- | --- | --- |
| 1. Audit | Inspect the named revision and the intended behavior. Write defects, evidence, affected scope, proposed fixes, which fixes already have approval, and which decisions are still missing. Make no production edits. | The audit, scope, and approvals are recorded, and one stage-2 thread has started. |
| 2. Implement | Branch from the pushed audit tip as `cursor/<slug>-stage2-<suffix>`. Apply only the approved fixes. Use the development configuration `X0X_DEV_PADCAL=1`. Run the scoped checks and commit. | Checked commits for the approved fixes are on the remote branch, deferred items are recorded, and one stage-3 thread has started. |
| 3. Test | Start from the pushed implement tip. Give the test run and any failure investigation to separate subagents. | The relevant tests pass, and each failure has a cause and a disposition. Then one stage-4 thread has started. |
| 4. Validate | Build Elite and Pro development packages from the tested candidate. Record exact package and controller identities, budgets, checks, and logs. | Both development packages meet the budgets. A host pass is not a package pass. Then one stage-5 thread has started. |
| 5. Merge | One integrator opens or updates the pull request into firmware `nightly` from the validated branch. Save the local install prompt. | The pull request points at the validated tip, and one stage-7 thread has started. |
| 6. Install | A local hardware thread installs the matching development artifacts and records installation and data-preservation checks. Stage 5 saves this prompt. Stage 6 starts no cloud thread. | A local install record exists for those artifact hashes. A cloud attempt with no device records `INSTALL BLOCKED` and leaves this stage pending. |
| 7. Publish nightly | Publish the valid development packages to the nightly installer page. | The live page serves that candidate, labeled for hardware test, and the served package hashes match. Stop. Do not open another audit. |

Use the current run's required branch prefix and suffix. Stages 1–5 each get their own `cursor/<slug>-stageN-<suffix>` branch. The successor works on its own new branch. The sender does not merge the successor's branch back into its own. Stage 7 starts from the pushed stage-5 branch and publishes in the nightly repository. Stage 6 runs from the saved prompt on a local machine.

Stage 4 stays on development packages. A full release gate runs only when the current request explicitly asks for a release build. Production `main`, production Pages, and a public release still need the owner's device pass for that same installed build, plus a separate production approval, in a fresh thread, and only when that evidence exists. This chain does not perform that promotion.

Before installation, compare the merged inputs with the stage-4 evidence. A new commit id by itself leaves matching evidence valid. A change to checked source, configuration, tools, dependencies, or artifacts sends the work back to a new thread at the earliest affected stage. Repeat a completed merge only when another merge is actually required. That new merge goes through the affected validation and merge stages again.

Stage 5 is the only integrator. Its pull request targets firmware `nightly`. Recheck open agents and that branch before shared changes so two integrators do not compete. Stage order does not add permission for production `main`, production Pages, or a new defect fix. Carry existing approvals forward.

Stages 1–5 each start one cloud successor. Stage 5's successor is stage 7, recorded in `SUCCESSOR-STAGE7.txt`. Stage 5 also saves the stage-6 local install prompt, including the package and controller hashes. That prompt is not a cloud thread. Stage 6 and stage 7 start no successor. A later local install records the device result and does not publish the page or push production `main`.

## Subagents inside the stage

Delegate the stage's real work to fresh subagents. Each helper is a new Task call with no `resume`. Use separate scopes: stage 3 uses one subagent for the test run and another for failure investigation. Other stages use a fresh subagent for the stage's checks or review.

A subagent supports the stage. It does not replace the stage thread, and it does not launch the next stage. Only the owner of stages 1–5 launches the successor.

## Successor handoff

When stage 1, 2, 3, 4, or 5 passes its checks, and before that owner stops:

1. Write `evidence/<slug>-stageN/HANDOFF.md`. Stages 1–4 also write `SUCCESSOR-STAGE{N+1}.txt`. Stage 5 writes `SUCCESSOR-STAGE7.txt` and the stage-6 local install prompt. The successor prompt names the next cloud stage, repository, pushed tip, branch, approved scope, evidence paths, completion condition, and this same successor rule. Stage 6 and stage 7 do not write a successor prompt and do not launch a thread.
2. Commit that handoff and push the stage branch. A cloud successor sees only the remote tip.
3. Check open cloud agents and the handoff record. If a successor for this stage already exists, record its id and stop. Do not start a second one.
4. Start exactly one next thread with the Task tool: `environment` `cloud`, `cloud_base_branch` set to the pushed stage branch, `run_in_background` true, and no `resume`. Put the full handoff in the prompt.
5. Write the returned agent id into `HANDOFF.md`, commit and push that id, then end. Do not wait for the next stage to finish.

If the tool returns no agent id, leave the successor pending in the handoff. Report it as unstarted. Do not claim it launched. Do not call the Cloud Agents HTTP API. Do not substitute a local helper for the next stage.

A failed check is not a completed stage. Repair it in a new thread of the earliest affected stage, or stay failed. An `INSTALL BLOCKED` cloud note is not a failed package check and is not an install pass. A source repair found in stage 3 or a package or source failure found in stage 4 returns to a new stage 2, then a new stage 3, then stage 4 again. The test thread does not patch the failure forward. Do not advance to merge, install, or the nightly page while checks are failed.

Missing hardware blocks the device install record only. Do not skip a failed package check and publish anyway. A valid candidate still publishes when no device is attached.

## Valid nightly page

Stage 7 runs after stage 4 and stage 5. It does not wait for a physical install. Follow the nightly repo's `docs/nightly-first-development-gate.md` and `docs/qualified-nightly-candidate.md`.

A candidate is valid only when every prehardware gate passes for the exact source tree: both the Elite and the Pro development packages, model and header identity, package and controller CRC and SHA-256, source attribution, memory and flash budgets with at least 8192 bytes of pool free, host regressions, backup, readback, and rollback, installer and model negatives, independent review, and a lawful distribution of the package bytes. The page label is `DEV / HARDWARE TEST — DEVICE NOT TESTED`. `hardware_tested` stays false.

Publish by linking those immutable hosted package bytes from the nightly installer. Fetch the bytes again and require the size and SHA-256 to match before the page can offer them. Reconcile the existing qualified catalog. Fast-forward the nightly repo's `gh-pages` through its publisher route, then fetch the live installer and confirm the served catalog, installer, and package hashes.

Leave `.fwsc` packages and `ctl.img` out of both Git repositories. Leave proprietary SDK, toolchain, DSP references, samples, and secrets out of the nightly repo. Leave production `main` and production GitHub Pages unchanged.

A failed, skipped, or missing gate is history only. Leave that candidate out of the flashable picker. If this thread cannot publish through the nightly Pages route, save the qualified handoff and report the page as not updated. A prepared folder is not a live page.

After the live page matches the candidate, stop. Do not open another audit, and do not treat the page as a production release.

## Local install

Stage 6 runs on a machine with the device attached. The stage-5 owner saves the local prompt and does not spawn a cloud installer.

A cloud thread that is asked to install writes `INSTALL BLOCKED`, the package and controller hashes, and the local prompt, then stops. It does not flash the device. The stage-7 publication thread is the only cloud successor of stage 5.
