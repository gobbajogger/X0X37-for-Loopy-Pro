# Workflow handoff to stage 3

Receiving stage: **3 Test** (new cloud thread).
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this prompt: `cursor/f02-loader-bind-stage2-fb90`.
Procedure: `AGENTS.md` and `docs/rules/workflow.md`.
Parent / stage-2 owner: `bc-3afa217a-8ab0-53d0-a0bb-8f745a71c2b2` (Project worker; `source=internal`).
Audited SHA: `13d32e1dee37b6fb5a1e339aff499a75eecb97b5`.
Implementation tip: this branch tip (includes F02 repair + evidence).
Repair commit: `af0ac4f90d7d2f1ec517fc7c2b582df7c0c8df9c`.
Checks: `evidence/f02-stage2/CHECKS.md`.
Deferred: `evidence/f02-stage2/DEFERRED.md`.
Successor prompt: `evidence/f02-stage2/SUCCESSOR-STAGE3.txt`.
Successor agent id: none.
Successor URL: none.
Handoff status: **incomplete**. `POST https://api.cursor.com/v1/agents` returned HTTP 401 (`Invalid User API Key`) with no `CURSOR_API_KEY` / `CURSOR_SERVICE_ACCOUNT_KEY` in this pod. Retried once; same result. Stage 2 still owns the loop. A Task subagent was not substituted. Pod identity token was not sent.

## Scope tested next

Stage 3 only. Verify the F02 fail-closed loader/reboot binding. Do not implement deferred items. Do not merge production `main`, publish production Pages, or flash a device.

When an owner-visible Stage 3 id is obtained (owner API key or desktop CreateAgent with `startingRef` `cursor/f02-loader-bind-stage2-fb90` and prompt `evidence/f02-stage2/SUCCESSOR-STAGE3.txt`), record id + URL here, commit, and push. Only then does Stage 2 ownership end.
