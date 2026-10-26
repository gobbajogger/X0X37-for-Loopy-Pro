# x0x37

Read this file first, then [docs/rules/workflow.md](docs/rules/workflow.md). That file is the procedure.

Optional host tooling gaps (Pillow, ASan, missing reference repos) are not cycle blockers. Install when possible (`bash tools/agent_env_install.sh` / environment `install`); otherwise mark those checks not-run and continue the loop. GitHub Actions and billing locks are never test results.

After local proof of specific heads, merge those Project PRs with `tools/merge_project_prs.py --pr N` (and/or `--head SHA`). Standing pre-approval covers only the proven set — do not batch-merge unrelated untested `cursor/` PRs. Automatic merge of properly tested work is pre-approved — no per-PR human APPROVE. Never ask the owner to approve a merge. Real conflicts are skipped and recorded. Stage 5 alone fast-forwards firmware `nightly`. Ruleset unlock for PRODUCTION main is done (required approvals 0, last-pusher approval off; force-push/delete protections kept). Do not ask the owner to unlock or approve again.

## Cursor Cloud specific instructions

Cloud Agent `install` must prepare Pillow, ASan runtime (`libclang-rt-18-dev`), Node on `PATH`, the JieLi toolchain (`tools/get_toolchain.sh` → `~/.jieli/toolchain`), AC79 SDK tag `AC79NN_SDK_V1.2.1_2023-12-13` with the three hashes in `cloud-handoff/2026-10-20/cloud_setup_20261020/firmware-setup-check.sh`, and pinned DSP refs under `~/x0x-refs` (also linked as `/schwung-*` for `tests/run_tests.sh` defaults). Login shells load `/etc/profile.d/x0x37-dev.sh` for `JIELI_TOOLCHAIN`, `AC79_SDK`, and reference paths.

Hello-world without hardware: `sh host/build_host.sh` then `build/host/x0x_host tests/scenarios/getting_started.x0x OUTDIR`. Development Elite package: `BOARD=smk37 MODEL=elite ./build.sh`. Run host/web suites with `sh tests/run_tests.sh` (the file may lack the executable bit). Missing optional refs or ASan stay not-run; they are not cycle blockers. No long-running `start` service is required for this tree.

