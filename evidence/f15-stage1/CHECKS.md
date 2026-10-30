# Stage 1 checks — F15 audit of 092589b8

| Check | Result | Evidence |
| --- | --- | --- |
| Live catalog `latest` | `nightly-092589b8-dev` MATCH handed SHA | `live-verify.log` |
| Elite/Pro page sha256 re-fetch | MATCH `3d107710…` / `60dc7fda…` | `live-verify.log` |
| ctl | `956e3e21…` | catalog |
| Label / hardware_tested | DEV / HARDWARE TEST — DEVICE NOT TESTED / false | `live-verify.log` |
| Deferred older tip FAIL? | none | `evidence/f13-stage2/DEFERRED.md` |
| `install_f03_slot_probe_test.py` @ blobs matching `092589b8` | **FAIL** exit 1 | `bf034-f03-slotprobe-witness.log` |
| `install_test.py` supporting fail-closed | **FAIL** (9) | `install-test-supporting.log` |
| `X0X_TEST_JOBS` | 8 | witness headers |
| GitHub Actions | not used | — |
| Selected defect | **BF-034** | `AUDIT.md`, `bf034-definition.md` |

Overall: audit recorded; defect yes → hand `SUCCESSOR-STAGE2.txt`.
