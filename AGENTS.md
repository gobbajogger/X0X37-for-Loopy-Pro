# x0x37

Read this file first, then [docs/rules/workflow.md](docs/rules/workflow.md). That file is the procedure.

After local proof of specific heads, merge those Project PRs with `tools/merge_project_prs.py --pr N` (and/or `--head SHA`). Standing pre-approval covers only the proven set — do not batch-merge unrelated untested `cursor/` PRs. Automatic merge of properly tested work is pre-approved — no per-PR human APPROVE. Never ask the owner to approve a merge. GitHub Actions and billing locks are not merge blockers. Real conflicts are skipped and recorded. Stage 5 alone fast-forwards firmware `nightly`. Ruleset unlock for PRODUCTION main is done (required approvals 0, last-pusher approval off; force-push/delete protections kept). Do not ask the owner to unlock or approve again.
