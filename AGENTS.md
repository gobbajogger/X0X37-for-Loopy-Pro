# x0x37

Read this file first, then [docs/rules/workflow.md](docs/rules/workflow.md). That file is the procedure.

After local proof, merge eligible Project PRs with `tools/merge_project_prs.py`. Automatic merge of properly tested work is pre-approved — no per-PR human APPROVE. Never ask the owner to approve a merge. GitHub Actions and billing locks are not merge blockers. Real conflicts are skipped and recorded. Stage 5 alone fast-forwards firmware `nightly`. Ruleset unlock for PRODUCTION main is done (required approvals 0, last-pusher approval off; force-push/delete protections kept). Do not ask the owner to unlock or approve again.
