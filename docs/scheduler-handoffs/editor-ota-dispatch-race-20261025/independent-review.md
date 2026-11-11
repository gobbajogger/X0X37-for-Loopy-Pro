# Independent editor/OTA dispatch review

Disposition: **ACCEPT for scoped source/host handoff. No source findings requiring correction.** This is not complete firmware QA, a target build, installable NIGHTLY qualification, or a hardware result.

Reviewed immutable commit `a8113c2a0036be76069fb6431454650ee84f6e73`, root `3c09d2698059657b3a0005bac944284970b56a1e`, firmware subtree `b7a004e4d37b95b4efc8def3842c9f41589cda06`; baseline `54f2ffbc3124f7449a69067dc8215a2d91a2e675`. Reviewer `/root/dispatch_review` did not author source or tests. Git archives of both exact commits were extracted into isolated temporary trees; source checkout stayed clean. Exact commands, exits and temporary tree locator are in `independent-runs.json`; reviewed blob IDs and log SHA256 values are in `independent-review-receipt.json`.

## Source reasoning

`ed_service` now reports eligibility only for a non-editor frame found by its original peek and left pending. Normal `fm1_main` invokes OTA only when that eligibility is true and the existing project hold permits it. The ISR cannot replace an already pending frame: `sx_ready` is cleared only by `ota_frame_done`, while collection begins only when the slot is free. A newly arriving editor frame after an empty editor check or after editor consumption therefore cannot be consumed by the later OTA check. It remains for the next editor pass.

The existing header classification is reused. Editor ordinary commands still copy before release; project/sample commands still keep the original frame through their handler. Project/recovery holds and update-request clearing remain intact. `ota.c`, `usb.c`, the OTA fixture, safe-mode and loader paths are unchanged. Sessions and safe-mode continue draining unrelated editor traffic, avoiding a new single-slot deadlock. Every normal-loop path assigns eligibility before use; there is no control-flow bypass to a stale value.

## Independently executed checks

| Check | Actual result |
| --- | --- |
| Same candidate dispatch regression against baseline source | Expected RED, exit 1: 22/34 checks pass, 12 fail |
| Candidate dispatch regression | GREEN, exit 0: 34/34 pass |
| Candidate dispatch regression, AddressSanitizer + UndefinedBehaviorSanitizer | GREEN, exit 0: 34/34 pass, no sanitizer report |
| Registered runner: editor-ota-dispatch, project-protocol, project-runtime | Exit 0, all 3 selected suites pass |
| Full project-protocol, ASan + UBSan | Exit 0 |
| Broader project-runtime, ASan + UBSan, candidate | Exit 134, existing `fastmath.h:57` left shift of negative -1 |
| Identical broader project-runtime sanitizer run, baseline | Exit 134, same `fastmath.h:57` failure |

Compiler: Apple clang 21.0.0, arm64-apple-darwin27.2.0. macOS runtime tests use `X0X_HOST_DEFS=-D_DARWIN_C_SOURCE=1` for existing libproc declarations; no source workaround was applied. The initial archive setup used an unavailable Python 3.9 tarfile argument and failed before any test; native tar extraction then succeeded. The broader sanitizer failure is independently shown pre-existing and does not invalidate the scoped sanitized dispatch result; full-root sanitizer qualification remains incomplete.

Regression coverage includes PING, addressed controller read offset 63392/count 224, project/sample and unknown editor commands arriving before/between/after consumers, late arrival after a prior editor release, exact once dispatch, copy/retain ownership, OTA identity, malformed checksum/short prefix/unknown drain, project/recovery holds, safe-mode drain and the actual session consumer. The editor handler is a sentinel. USB reception, ISR timing probability, actual editor replies, full sessions/flash/package handling and devices are not exercised.

The historical hardware timeouts remain unattributed. No GUI, hardware I/O, firmware target build, device configuration, capture, reset, reflash, publication, commit or push was performed by this reviewer. Target compilation/code/RAM budgets, complete combined-root QA, integration and model-specific hardware qualification remain separate owner gates.
