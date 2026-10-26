---
name: x0x37-workflow
description: "Use the requested tstack review and PR mode for x0x37 code changes, with project validation and permission boundaries. Excludes docs-only tasks."
---

# Code and PR workflow

Use the pinned tstack skills as the workflow baseline. Select a mode from the owner's current request; installation alone does not activate a mode:

- **Sol Loop:** Read and follow `sol-loop`. Its reviewer must actually run GPT Sol. If that route is unavailable, report the review as blocked; never rename another reviewer Sol or claim agreement.
- **Full Send:** Read and follow `full-send` and its dependencies only when explicitly requested. Its completion target is a merged PR, subject to the project boundaries below.
- **Full Stop:** Read and follow `full-stop` and its dependencies only when explicitly requested. Leave the PR open for the owner; do not merge or enable auto-merge.
- **Ordinary work:** Finish the authorized change and applicable proof. Use `file-pr`, `babysit-pr`, or `leaving-pr-comments` when the request calls for their action. Do not infer Full Send or Full Stop.

For code work, use the tstack order: finish the change, prove the changed behavior, get the requested Sol review, file the PR, handle checks and real feedback, then apply the selected mode's merge or handoff. Do not run Full Send and Full Stop together. Keep the diff small; a reviewer finding is evidence to verify, not an automatic scope expansion. Reuse exact-input proof where valid. Read [project stages](references/stages.md) for firmware-specific validation.

## Project boundaries

The owner's current instructions outrank generic grants in upstream skills. No current hardware permission: no device probe, install, flash, recovery, or GUI. Full Send or Full Stop does not restore it, and upstream's expanded test-authority wording does not authorize those methods here. Preserve sample, project, calibration, and backup data.

No force push, including `--force-with-lease`; no merge, auto-merge, queue change, production promotion, publication, access change, or release-catalog edit under this workflow task's current grant. A future explicit mode request still needs the owner's applicable permission for those actions. Do not silently disable another person's auto-merge. Read live PR state and report a conflict before touching that setting.

Firmware `main` remains hardware proven on exact bytes. Keep the assigned integrator as sole owner of firmware integration and branch order. A host PASS is not a native package PASS or device PASS. Source and artifact identities, applicable Elite/Pro checks, independent QA, and failed-check disposition remain required. Never call a BUILD_ONLY package installable or DEV CHECKED.

Keep an active owner and an exact next checkpoint for unfinished work. A missing tool or reviewer ends that operation with a truthful blocker; continue independent authorized work. Do not create an automatic stage or audit loop. Use [handoff](references/handoff.md) only when a real receiver is needed.
