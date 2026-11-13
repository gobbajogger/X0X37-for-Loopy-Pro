# BF-007: loading legacy project loses 909/808 Bass Drum PAN motion mapping

**2026-10-23 | Severity MEDIUM | AUDIT ONLY — SOURCE UNCHANGED, NO DEVICE PASS**

On current main `c9718ae833eb64a5191c350922593df8a36017c9`, `firmware/src/app/project.c` (Git blob `1000a0d88e04553295b82d724a63670434b67748`), `sound_bd_sends()` at lines 493–504 attempts to migrate BOTH BD PAN sound values and the matching motion lane index; `read_project()` invokes it at lines 549–556 immediately after OBJ_SOUND. However, the legacy motion lanes and compressed arrangement are loaded only afterward at lines 563–578. The lane migration sees only defaults. Old projects therefore retain motion index 8 for 909 (expected 10) and 7 for 808 (expected 9), now corresponding to send controls. Static PAN values migrate successfully. Old projects with automated BD PAN may instead automate sends; no physical observation claimed.

**Reproduction:** Re-used a historical offline synthetic tree `f58d62f0b622b9c5792bda81793e0031d8be8747`; NOT the entire current main checkout. Checked all sixteen compiled source and test dependencies by exact Git blob SHA against immutable live main, including `project.c`, `engine.c`, `project_stage_test.c` (`199f62a33669de901ca0f95b64ed835e272ce6f0`), and `run_project_stage.sh` (`32c5481b81adcea17b730628dd1dc7c12272eef2`). New attached C regression uses actual project_save/project_load_stage, mock NOR and compressed storage; covers snd_rev=2/3 and current-version control. The test is intentionally **not integrated into a passing default gate** while main remains broken.

- Existing unchanged project-stage test: PASS (24,446 checks; no migration scenario).
- New regression, unchanged main-matching source, GCC 14.2 ASan/UBSan: exit **1**, 171 checks / 3 FAIL (909 legacy rev2, 808 legacy rev2/3).
- Same regression, unchanged source, Clang 17 ASan/UBSan: exit **1**, same failures.
- Scratch-only repair: split sound-value migration from lane migration, call lane migration only AFTER arrangement load while leaving static PAN migration at its existing location. GCC+ASan/UBSan and Clang+ASan/UBSan: **171/171 PASS each**.
- Scratch-only existing named project-protocol, project-stage, project-runtime host suites: all executed and PASS. No unregistered tests or package skips counted as PASS.
- Local evidence archive SHA256: `ad399f75966801332a6fa2be2f418f998a373241da67bb09da5b13ef37bc4ba6`; includes per-case logs, diff, executable C regression and SHA256SUMS.

The candidate project.c repair was NOT pushed. Current target-build prerequisites/controller/Elite/Pro proof unavailable; no firmware compile, flash, sample operation, release, or Pages update. Owner's sole firmware integrator must review and validate. SMC, Pages, installer PR owners unaffected. No new user-visible improvement this run.