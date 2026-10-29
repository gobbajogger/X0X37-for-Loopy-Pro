# x0x37

Read this file first, then [docs/rules/workflow.md](docs/rules/workflow.md). That file is the procedure. For stage PRs and local-env review, use the minimum proof in [docs/rules/local-proof.md](docs/rules/local-proof.md).

Optional host tooling gaps (Pillow, ASan, missing reference repos) are not cycle blockers. Install when possible (`bash tools/agent_env_install.sh` / environment `install`); otherwise mark those checks not-run and continue the loop. GitHub Actions, billing locks, and `@codex` / Codex review findings are never test results and never merge blockers after local proof.

After local proof of specific heads, merge those Project PRs with `tools/merge_project_prs.py --pr N` (and/or `--head SHA`). Standing pre-approval covers only the proven set — do not batch-merge unrelated untested `cursor/` PRs. Automatic merge of properly tested work is pre-approved — no per-PR human APPROVE. Never ask the owner to approve a merge. Real conflicts are skipped and recorded. Stage 5 alone fast-forwards firmware `nightly`. Ruleset unlock for PRODUCTION main is done (required approvals 0, last-pusher approval off; force-push/delete protections kept). Do not ask the owner to unlock or approve again.

No retest of proven heads: if the same commit and artifact hashes already have valid local proof, confirm the record matches, then merge or publish. Retest only when the head, artifacts, or claimed tests changed.
