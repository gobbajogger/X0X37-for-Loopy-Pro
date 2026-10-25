# Evidence adapter

Use the canonical [small stage packet](../../../../docs/engineering-delivery/handoff.md)
and [contract](../../../../docs/engineering-delivery/SKILL.md). This glossary names
observations only; it cannot override acceptance or authorization. Authentic existing
evidence can satisfy a field without a new redundant form. Unknown optional metadata
is not a blocker. Keep private retrieval locations in the authorized private handoff.

| Label | Observation |
| --- | --- |
| `SOURCE ONLY` | Source is pushed. Host checks have not run. |
| `HOST CHECKED` | The applicable host checks exited 0 on this commit. |
| `BUILD_NOT_RUN` | The compiler was not started. |
| `BUILD_FAILED` | The compiler exited non-zero, or a budget check failed. |
| `BUILD_SUCCEEDED_UNREVIEWED` | The compiler exited 0. Independent QA has not accepted this commit. This label is not the firmware nightly gate. |
| `UNCHANGED_VALID_SCOPED` | Compiled inputs match an attested build. The old package hashes are reused. |
| `SCOPED FIRMWARE-SUBTREE BUILD` | A subtree was built. Full-root QA is still pending. |
| `DEV CHECKED` | `tools/dev_validate.py` wrote a record whose result is `DEV CHECKED` for these exact artifacts. It is not independent QA or a source-merge decision. |
| `QA_PENDING` | Independent QA has not accepted this exact candidate; required independent QA remains incomplete. |
| `QA_PASSED` | A reviewer who did not author the change accepted this exact commit. |
| `NO-GO` | A required check failed. |
| `PACKAGE_NOT_PUBLISHED` | The package is not on the live installer. |
| `SITE BRANCH ONLY` | `gh-pages` moved. The live bytes have not been fetched. |
| `DOWNLOAD_HTTP_VERIFIED` | The fetched installer bytes match the qualified package. |
| `DEVICE NOT TESTED` | There is no attributable hardware session for these bytes. |
| `DEVICE VERIFIED` | An install record for the exact package and controller hashes, on the matching model, with backup, readback, and the running identity checked. `INSTALL BLOCKED` is not this label and is not success. Elite evidence does not move a Pro package, and Pro evidence does not move an Elite package. |
| `GIT DELIVERY PENDING` | The commit is only local, or the push was refused. |
| `NO_NEW_DELIVERABLE` | No eligible change. No empty commit and no empty pull request. |
| `DELIVERY_BLOCKED` | The required write was refused. |
| `EXACT-SOURCE VALIDATION PENDING` | The exact tree could not be reconstructed. |

Optional `X0X37-Role`, `X0X37-Stage`, `X0X37-Consumer`, `X0X37-Evidence` and
`X0X37-Next` trailers can help readers. They are not required metadata or a reason
for a trailer-only commit. The optional ledger introduced in PR #134 is another
evidence adapter, not a universal execution or acceptance gate.
