# x0x37

Read this file first, then [docs/rules/workflow.md](docs/rules/workflow.md). That file is the procedure.

After local proof, merge eligible Project PRs with `tools/merge_project_prs.py`. Automatic merge of properly tested work is pre-approved — no per-PR human APPROVE. Never ask the owner to approve a merge. GitHub Actions and billing locks are not merge blockers. Real conflicts are skipped and recorded. Stage 5 alone fast-forwards firmware `nightly`. If the PRODUCTION main ruleset still blocks agent merges, record the one-time unlock (`tools/merge_project_prs.py --print-onetime-setup`) once; do not open a For Review “please approve” loop every PR.
