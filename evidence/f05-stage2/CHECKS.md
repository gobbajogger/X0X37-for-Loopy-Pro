# Stage 2 checks for F05 (BF-002)

Branch: `cursor/f05-bf002-stage2-fb90`
Stage-2 owner: `bc-9490d387-26c4-58f1-a59e-84ccb8145ef7`
Base audit tip: `2a3c6ce82c488c6d07cf902d484a64e31d43aee6` (`cursor/f05-bf002-stage1-fb90`)
Audited / validated source SHA: `0d79d63485dc6585800a71ef716d095445f5a1fc`
Content tree: `1b067cadd1e7887415354c4f384a51f93d6437c367574e2cee52b0b44d829479`
Git tree: `603e2a2b1442a8214cd97b03d32bf57fc158feac`
Repair: **BF-002 only** (planner project-suite routes). No firmware / controller / package bytes changed.
No production `main` / production Pages update. No device flash. GitHub Actions were not used as tests.

## Local proof (this head)

| Check | Result | Log |
| --- | --- | --- |
| `python3 -m unittest tests.host.test_quickcheck_plan -v` | **OK** (11 tests, including `test_project_web_routes_select_project_suites`) | `evidence/f05-stage2/planner-host.log` |
| `X0X_TESTS='projects-web projects-page' bash tests/run_tests.sh` | **PASS** (`ok projects-web`, `ok projects-page`) | `evidence/f05-stage2/projects-suites.log` |
| Offline `select()` probe for project modules | `projects-web` + `projects-page` **selected** for `web/x0x_projects.js`, `web/x0x_projects.html`, `web/x0x_breaks.js` | `evidence/f05-stage2/bf002-verify.log` |
| Offline `select()` for unrelated UI (`web/x0x_installer.html`) | project suites **not** selected | same |

## Repair summary

`tools/quickcheck_plan.py` adds explicit `TEST_ROUTES` for project JS/HTML, shared `web/x0x_breaks.js`, and the project test entrypoints so scoped quickcheck includes `projects-web` / `projects-page`. Host fixture in `tests/host/test_quickcheck_plan.py` advertises both project commands and asserts production-module edits select both suites while unrelated UI stays scoped.

## Overall

**IMPLEMENT CHECKED.** Ready for Stage 3.
