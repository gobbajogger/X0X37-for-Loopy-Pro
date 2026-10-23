# Scheduler Git deliverables and build-evidence contract

Owner decision: 2026-10-23. This is a **development-process draft for the existing hourly LoopyX0X37 schedulers**, not authority to merge to production, flash hardware, publish binaries, force-push or bypass repository checks. The current Classic Build plugin reference `references/git-run-deliverables.md` and this document describe the same mandatory workflow; current AGENTS/BUILDING/LICENSING and owner protections prevail.

## Each enabled run must leave an accountable outcome

At start, inspect live source+website repository SHAs, rules, actual producer/QA PRs, source tree and the role's last consumed evidence. Select the highest-impact unowned safe task and do actual feasible work.

- **DELIVERED:** produce substantive code, scoped regressions, measured optimization, build manifest, catalog/history update, test report or *new reproducible findings* on the **worker-owned isolated development lane**. Commit **source + findings summary + checked evidence**, fast-forward push, re-fetch exact remote SHA and expose the existing/new draft PR. Record `docs/scheduler-handoffs/<issue>.json` conforming to `docs/scheduler-evidence.schema.json` for nontrivial work. A verified WIP/NO-GO finding is deliverable evidence, **never a passed build**.
- **REVIEW_DELIVERED:** Independent QA, merger, publisher or throughput auditor can use an exact-SHA-linked Git PR review/comment/merge/deploy record as the durable Git output, with actual command exits, findings and next owner. They must not overwrite producer lanes, self-approve or churn duplicate report commits. Substantive standalone analysis may be committed to an owned evidence lane with the JSON record.
- **NO_NEW_DELIVERABLE:** there was no new meaningful safe work after actual checked queue/attempts; cite the unchanged source SHA and prior still-valid result. **Do not** create blank, boilerplate, duplicate or untruthful commits just to satisfy an hourly quota.
- **DELIVERY_BLOCKED:** a specific new attempted source/write/verification failed; record the exact failed tool/command, remote HEAD, incomplete criterion and next safe owner. A denied write is denied; do not bypass through a different API/branch/payload. Local-only work is not remote-delivered.

After a checked item, continue with another independently safe scoped item when feasible during the bounded run. Never treat a scheduled start minute as proof an earlier task finished. Git is the primary handoff queue (repo, branch, full commit SHA, evidence revision), not chat transcript messages.

## Minimum structured findings / handoff

Commit a JSON record on the **same owned lane** as the source/manifest change, describing the role, UTC run, exact *tested* repository/ref/full source commit/tree, concise findings (including failed negative paths), produced file/log paths and linked PR, executed commands/exit codes/log hashes/coverage, target device/firmware status, outstanding guards, next worker and concrete acceptance operation. The evidence commit cannot embed its own future SHA; put the verified committed head SHA into its PR/consumer handoff after push. Source SHA and artifact SHA256 are different identities.

Each `PASS` test needs an executed command and exit 0; `FAIL` retains exit+finding; `NOT_RUN`/`BLOCKED` need the specific reason, not a synthetic PASS. Producer tests are not independent QA. Never label a source review, historical binary, GitHub Actions zero-job/startup failure, site preview or Git branch as a native build/physical-device/HTTP deployment PASS.

## Real Elite/Pro builds and artifact storage

The **dedicated Hourly Build** role must attempt true pinned *Elite and Pro DEV* builds plus matching controller on each changed, accepted, exact integrated firmware source SHA where all required inputs are present. Record controller/header/model/CRC/SHA256/bytes, toolchain/SDK/reference pins, build commands/exits, flash/RAM/pool limits, data backup/readback/restore/interruption and installer tests, QA review and each model's result. Preserve a previous genuinely verified byte-identical candidate on unchanged/failed runs; do **not** demand repeated compilation just to populate a log.

**Commit the build findings, manifest, actual source/test changes and immutable artifact references to Git, not automatically the firmware binary.** This firmware repository and the distinct public `gobbajogger/x0x37-for-Loopy-Pro_nightly` repository have separate obligations. The NIGHTLY browser/status-only repository must **never** contain firmware `.fwsc`, `ctl.img`, vendor SDK/JieLi tools, DSP references, samples, backups or secrets. Binary storage/distribution needs its own lawful, authorized destination with complete applicable license/source and independently checked served bytes. A hash-only manifest cannot make a package installable.

Only sole Safe Merge integrates firmware to `nightly` by approved reviewed PR flow; only independent QA validates producer claims; only the NIGHTLY Main Merger integrates reviewed website PRs; only the sole NIGHTLY Pages publisher updates permitted site content and verifies remote Git **and served HTTP bytes**. NIGHTLY DEV/HARDWARE TEST candidates may be DEVICE NOT TESTED when full software/data/model/QA/licensing/hosting gates pass, without prior owner physical PASS or per-build permission. PRODUCTION main/picker require same-byte model-specific owner hardware PASS and separate approval.

## Required consumer behavior

Fetch actual producing SHA, tree, changed files and evidence manifest. Verify checks and underlying artifact bytes independently before advancing, mark `REVIEWED` / `NEEDS CHANGES` / `INCOMPLETE`, and leave a SHA-linked Git disposition. Pipeline Throughput counts delivered vs unconsumed PR/QA/build/catalog/Pages/HTTP checkpoints, reports queue age and named next owner, and flags unsubstantiated claims as **UNDELIVERED**. Do not force empty commits when the correct output is a review comment or no new work.
