# Full-cycle dev chain

Use this procedure for an authorized firmware or code change. Each stage is its own thread. Helpers do the work inside that thread. Stages 1–5 and stage 7 advance by starting exactly one owner-visible next-stage agent (this pod, or the Project coordinator when this pod cannot create agents). Stage 6 starts none.

The owner's current request overrides this procedure. Otherwise the cloud process does not pause and does not end after one cycle. A cycle publishes a valid nightly build to the nightly installer page, <https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/>, then stage 7 starts the next cycle's audit. That page is the public preview in `gobbajogger/x0x37-for-Loopy-Pro_nightly`. It is not production `main` and not production GitHub Pages.

A docs-only edit does not start the stage loop. A new job is still a new thread. This procedure replaces one-chat stage routing. Leave the removed skill pack, ChatGPT project routing, and Codex `create_thread` instructions unrestored.

Every change states a reason, a purpose, and the effect on the person using the device.

Agents run every applicable test in their own environment with the tools that machine has. Optional host tooling gaps (Pillow, ASan, missing reference repos) are not cycle blockers. Install when possible (environment `install` / `bash tools/agent_env_install.sh`); otherwise mark those checks not-run and continue the loop. A generator that actually runs and fails still fails. GitHub is not a test runner. GitHub Actions, billing locks, and `@codex` / Codex review findings are never test results and never merge blockers after local proof. A missing optional input does not stop other work. Do not wait on another agent, on GitHub, or on the owner for a missing optional library. Do the next independent step.

For stage PRs and local-env review, use the minimum proof that still decides merge or publish: [local-proof.md](local-proof.md). Plan selectors with `python3 tools/local_proof_plan.py --base <pr-base> --head HEAD`. If the exact commit and artifact hashes already have valid local proof records, verify that proof matches the head — do not re-run those selectors. Retest only when the head, artifacts, or claimed selectors changed. Evidence-only tips verify CHECKS and recorded digests; they do not rebuild Elite/Pro packages when stage 4 already recorded `DEV CHECKED`. Keep package integrity gates (`dev_validate`, `pool_free`, publish hash MATCH) and required BF host regressions when firmware changes and prior proof does not already cover that exact head.

## Project PR merge

When Project PRs are mergeable after local proof, merge them. Do not leave them parked in For Review. Automatic merge of properly tested work is pre-approved by the owner — no per-PR human APPROVE, and never ask the owner to approve a merge.

Use `tools/merge_project_prs.py --pr N` (and/or `--head SHA`) after local proof of those exact heads (prefer `--dry-run` first). Prove-merge means verify existing proof matches the head and claimed hashes, then merge — not a fresh kitchen-sink retest. Standing pre-approval does not merge unrelated untested `cursor/` PRs. The tool walks the proven set bottom-up, uses a normal merge commit (no force-push), skips real conflicts and records the skip, and treats GitHub Actions / billing / Codex failures as non-blocking. Prefer GitHub auto-merge when the API allows it; if auto-merge is armed but required Actions failed (billing), fall through to direct/`--admin` merge. After a stack parent merges, retarget remaining children to the parent's base (usually `main`) before merging them so the tip reaches trunk.

This path does not race stage 5. Stage 5 remains the only integrator that fast-forwards firmware `nightly`. Do not merge into `nightly` here. Do not merge production `main` from unvalidated firmware bytes; firmware `main` moves only after a successful hardware validation of those exact bytes.

Ruleset `LoopyX0X37 PRODUCTION main - reviewed PR only` is unlocked for agent merges: required approvals are 0 and last-pusher approval is off. Force-push and delete protections stay. Do not ask the owner to unlock or approve again. `tools/merge_project_prs.py --print-onetime-setup` only records that the unlock is done.

## One fresh thread per stage

The cloud stages are a loop: stage 1, then 2, then 3, then 4, then 5, then 7, then stage 1 again. The current stage creates the next thread only after its own checks pass, and only the next thread. It does not create the rest of the loop in advance. Stage 6 is the local install prompt saved by stage 5. It is not a cloud thread between stage 5 and stage 7, and stage 7 does not wait for it. The thread assigned to a stage does that stage. It does not create a second copy of itself.

Never pass `resume` to turn a stage agent into the next stage. A stage change needs a new thread.

| Stage | Starting state and goal | Done when |
| --- | --- | --- |
| 1. Audit | Inspect the named validated SHA and the intended behavior. Write defects, evidence, affected scope, proposed fixes, which fixes already have approval, and which decisions are still missing. Make no production edits. | The audit is recorded and the successor path has started (see Successor handoff). A recorded defect starts stage 2. No recorded defect on an unchanged published tip → WAIT_FOR_TIP (or park/subscribe); do not open an empty evidence PR. When the tip later moves, start the next stage 1. The loop does not end. |
| 2. Implement | Branch from the pushed audit tip as `cursor/<slug>-stage2-<suffix>`. Apply only the approved fixes. Use the development configuration `X0X_DEV_PADCAL=1`. Run the scoped checks and commit. | Checked commits for the approved fixes are on the remote branch, deferred items are recorded, and one stage-3 thread has started. |
| 3. Test | Start from the pushed implement tip. Give the test run and any failure investigation to separate subagents. | The relevant tests pass, and each failure has a cause and a disposition. Then one stage-4 thread has started. |
| 4. Validate | Build Elite and Pro development packages from the tested candidate. Record exact package and controller identities, budgets, checks, and logs. | The affected development packages are DEV CHECKED and meet the budgets. A host pass is not a package pass. Then one stage-5 thread has started. |
| 5. Merge | One integrator fast-forwards firmware `nightly` when the host checks passed and the affected development packages are DEV CHECKED, and saves the local install prompt. | The handoff names that validated SHA. `nightly` matches it, or the refusal is recorded without a force-push. One stage-7 thread has started on that same SHA. |
| 6. Install | The Mac LaunchAgent `com.x0x37.unattended-install` may run the saved stage-6 prompt for a validated development package and record installation and data-preservation checks. Stage 5 saves this prompt. Stage 6 starts no cloud thread. | A local install record exists for those artifact hashes. A cloud attempt with no device records `INSTALL BLOCKED` and leaves this stage pending. |
| 7. Publish nightly | Publish the valid development packages to the nightly installer page. Then start the next cycle. | The live page serves that candidate, labeled for hardware test, and the served package hashes match. One new stage-1 thread has started. The page is not a production release. |

Use the current run's required branch prefix and suffix. Stages 1–5 each get their own `cursor/<slug>-stageN-<suffix>` branch. The successor works on its own new branch. The sender does not merge the successor's branch back into its own. Stage 7 starts from the pushed stage-5 branch and publishes in the nightly repository. Stage 6 runs from the saved prompt on a local machine.

Stage 4 stays on development packages. A full release gate runs only when the current request explicitly asks for a release build. Firmware `main` fast-forwards after a successful hardware validation of those exact bytes on the matching model. That replaces the separate production-approval wait. `INSTALL BLOCKED` is not success. Elite evidence does not move a Pro package. Production GitHub Pages is not this merge. No force-push.

Before installation, compare the merged inputs with the stage-4 evidence. A new commit id by itself leaves matching evidence valid. A change to checked source, configuration, tools, dependencies, or artifacts sends the work back to a new thread at the earliest affected stage. Repeat a completed merge only when another merge is actually required. That new merge goes through the affected validation and merge stages again.

Stage 5 is the only integrator of firmware `nightly`. It fast-forwards firmware `nightly` when the host checks passed and the affected development packages are DEV CHECKED. If the remote refuses the update, record the refusal and do not force-push. Stage 7 and the next audit still use that validated SHA, not an older `nightly` commit. Stage order does not add permission for production GitHub Pages or a new defect fix. Carry existing approvals forward.

Stages 1–5 and stage 7 each start one cloud successor. Stage 5's successor is stage 7, recorded in `SUCCESSOR-STAGE7.txt`. Stage 5 also saves the stage-6 local install prompt, including the package and controller hashes. That prompt is not a cloud thread. Stage 6 starts no successor. Stage 7's successor is the next cycle's stage 1, recorded in `SUCCESSOR-STAGE1.txt`. A later local install records the device result and does not publish the page or start a cycle.

## Subagents inside the stage

Delegate the stage's real work to fresh subagents. Each helper is a new Task call with no `resume`. Use separate scopes: stage 3 uses one subagent for the test run and another for failure investigation. Other stages use a fresh subagent for the stage's checks or review.

A subagent supports the stage. It does not replace the stage thread, and it does not launch the next stage. A Task subagent is internal to the current run. The owner cannot open it. Only the owner of stages 1–5 and stage 7 launches the successor, and that successor is an agent the owner can open.

## Successor handoff

A stage advances when an owner-visible Cloud Agent for the next stage is running (or has been launched) against the pushed successor prompt. A Task subagent is never a handoff. Prefer recording the successor id and `https://cursor.com/agents/<id>` URL on the remote when this stage can create or already sees that agent.

When stage 1, 2, 3, 4, 5, or 7 passes its checks:

1. Write `evidence/<slug>-stageN/HANDOFF.md` and the successor prompt. Stage 1 with a recorded defect writes `SUCCESSOR-STAGE2.txt`. Stage 1 with no recorded defect on an unchanged tip writes `SUCCESSOR-STAGE1.txt` (or a WAIT_FOR_TIP note) and does not open an empty evidence PR. Stages 2–4 write `SUCCESSOR-STAGE{N+1}.txt`. Stage 5 writes `SUCCESSOR-STAGE7.txt` and the stage-6 local install prompt. Stage 7, after the live page matches, writes `SUCCESSOR-STAGE1.txt` and returns the loop to audit. A stage-7 thread that could not update the live page writes a fresh stage-7 prompt instead, and does not start stage 1 yet. The prompt names the next stage, repository, validated SHA, branch, approved scope, evidence paths, completion condition, and this same rule. Stage 6 does not write a successor prompt and does not launch a thread.
2. Commit and push that prompt. The new thread can see only the remote tip.
3. Read the remote handoff and the owner's cloud agents. A recorded successor counts when its source is one the owner can open (web, mobile, desktop, api, slack, linear, automations, or a Project coordinator launch) and its URL is `https://cursor.com/agents/<id>`. If this handoff already has that kind of id, keep it and do not start another. An agent whose source is internal does not count, even when this run can see it. A running thread from an earlier cycle does not count either.
4. If no owner-visible successor is recorded, create exactly one with `POST https://api.cursor.com/v1/agents`. Authenticate with the owner's user or service-account API key. Send the repository `https://github.com/gobbajogger/X0X37-for-Loopy-Pro`, `startingRef` set to the pushed branch, and the full successor prompt. The pod identity token is not that key. Do not send it.
5. When create succeeds, write its id and URL into `HANDOFF.md` only after its source is owner-visible, then commit and push that record. The current stage then ends. It does not wait for the next stage to finish.

If the create call has no API key, is rejected, or returns only an internal subagent: push the successor prompt, record `ASK_COORDINATOR` (or equivalent) with the exact prompt path and `startingRef`, and stop this stage. A Project coordinator (or other owner-visible launcher) that starts that successor **satisfies** the handoff — do not open review threads demanding a pod-pushed CreateAgent id, and do not treat the missing key as a merge blocker. Do not claim this pod launched the agent. Do not substitute a Task subagent, a local helper, or a pod token. Retry create once when a key is present; a second failure follows the same ASK_COORDINATOR path.

A failed check is not a completed stage. Start one new thread at the earliest affected stage. An `INSTALL BLOCKED` cloud note is not a failed package check and is not an install pass. A source repair found in stage 3, a package or source failure found in stage 4, or a prehardware gate that fails in stage 7 returns to a new stage 2, then a new stage 3, then a new stage 4. The test thread does not patch the failure forward. Do not advance to merge, install, or the nightly page while checks are failed. Do not leave the failure without a successor.

Missing hardware blocks the device install record only. Do not skip a failed package check and publish anyway. A valid candidate still publishes when no device is attached.

## Valid nightly page

Stage 7 runs after stage 4 and stage 5. It does not wait for a physical install. Follow the nightly repo's `docs/nightly-first-development-gate.md` and `docs/qualified-nightly-candidate.md`.

A candidate is valid only when every applicable prehardware gate passes for the exact source tree: both the Elite and the Pro development packages, model and header identity, package and controller CRC and SHA-256, source attribution, memory and flash budgets with at least 8192 bytes of pool free, host regressions, backup, readback, and rollback, installer and model negatives, and independent review. Owner policy: a missing distribution-license, corresponding-source, or native-build receipt does not block a candidate. Do not invent an MIT claim for the tree; follow LICENSE and LICENSING.md for what the code is under. A present receipt that is not PASS, a hash mismatch, a failed package check, or a compiler failure stops that candidate only. The page label is `DEV / HARDWARE TEST — DEVICE NOT TESTED`. `hardware_tested` stays false.

Publish by linking those immutable hosted package bytes from the nightly installer. Fetch the bytes again and require the size and SHA-256 to match before the page can offer them. Reconcile the existing qualified catalog. Fast-forward the nightly repo's `gh-pages` through its publisher route, then fetch the live installer and confirm the served catalog, installer, and package hashes.

Leave `.fwsc` packages and `ctl.img` out of both Git repositories. Leave proprietary SDK, toolchain, DSP references, samples, and secrets out of the nightly repo. This nightly publish leaves production `main` and production GitHub Pages unchanged.

A failed or skipped applicable check stays out of the flashable picker. Record that history, and start one stage-2 repair thread for that failure. Do not publish the candidate and do not start stage 1 for a new defect in parallel. If this thread cannot publish a candidate that already passed every applicable gate, save the qualified handoff and start one fresh stage-7 retry of that same candidate under the successor rule. A successor id already on this handoff, or a thread already running for this handoff's stage-7 prompt, blocks a second start. An older retry does not. Do not start stage 1 from a page that did not update. A prepared folder is not a live page.

After the live page matches the candidate, start the next cycle as specified below. Do not treat the page as a production release.

## Next cycle

This section applies only after the live page matches the candidate. Stage 7 then starts exactly one fresh stage-1 thread, pushes that thread's id, and only then ends its own turn. It does not wait for a device install, an owner message, or a new request. The successor prompt names the validated SHA that was published, the page hashes, and this procedure. `cloud_base_branch` is the pushed branch that contains that SHA and the handoff. The audit uses that SHA even when firmware `nightly` is behind it.

The next defect is the oldest deferred defect from any cycle that is not already fixed in that SHA, then a recorded failing check that is not already fixed in that SHA, then the oldest open audit or bugfinder finding not already contained in that SHA. The new audit does not ask which defect to take. It does not resume stage 7. The duplicate check is the successor id for this handoff, not an older cycle's stage-1 id. If this handoff's stage-1 id is already on the remote, or that successor is already running, stage 7 keeps that id and does not start a second thread. The handoff finishes only when the id is pushed.

When no such defect is recorded, the audit records that unchanged SHA and starts one new stage-1 thread under the handoff rule. It does not start stage 2. That return to audit is the loop. A previous cycle's audit id does not block it. A failed page update stays on a stage-7 retry until the page matches, and that retry then returns to stage 1. A failed package or prehardware gate returns through stage 2, stage 3, and stage 4, then rejoins the loop at stage 5 and stage 7.

## Local install

Stage 6 runs on a machine with the device attached. The stage-5 owner saves the local prompt and does not spawn a cloud installer. A cloud agent does not flash. The Mac LaunchAgent `com.x0x37.unattended-install` may run the saved stage-6 prompt for a validated development package.

A cloud thread that is asked to install writes `INSTALL BLOCKED`, the package and controller hashes, and the local prompt, then stops. A successful install record fast-forwards firmware `main` as the hardware-validation rule above states. The stage-7 publication thread is the only cloud successor of stage 5.
