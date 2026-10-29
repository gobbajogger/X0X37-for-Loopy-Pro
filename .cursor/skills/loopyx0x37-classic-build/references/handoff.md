# Evidence packet and handoff

Git is the cross-run inbox. A chat summary, a subagent id, and a local patch are not delivery.

## Packet

Every source delivery includes:

- repository, branch, full 40-character commit, full root tree, and compiled subtree when a build ran
- producer role and the next consumer
- commands, exits, and the checks that failed, were not run, or are unknown
- for a build: per-model config, toolchain and SDK pins, package and controller sizes, SHA-256, CRC, budgets, and the private retrieval location of the bytes
- pull request URL after it exists
- the next permitted action and the gate that is still open

The evidence commit cannot contain its own future hash. Report that hash after the push, in the pull-request body.

Use a glossary name only for the observation it names. A negative result stays visible.

| Label | Observation |
| --- | --- |
| `SOURCE ONLY` | Source is pushed. Host checks have not run. |
| `HOST CHECKED` | The applicable host checks exited 0 on this commit. |
| `BUILD_NOT_RUN` | The compiler was not started. |
| `BUILD_FAILED` | The compiler exited non-zero, or a budget check failed. |
| `BUILD_SUCCEEDED_UNREVIEWED` | The compiler exited 0. Independent QA has not accepted this commit. |
| `UNCHANGED_VALID_SCOPED` | Compiled inputs match an attested build. The old package hashes are reused. |
| `SCOPED FIRMWARE-SUBTREE BUILD` | A subtree was built. Full-root QA is still pending. |
| `DEV CHECKED` | `tools/dev_validate.py` wrote a record whose result is `DEV CHECKED` for these exact artifacts. |
| `QA_PENDING` | This run authored the change, so it cannot supply independent QA. |
| `QA_PASSED` | A reviewer who did not author the change accepted this exact commit. |
| `NO-GO` | A required check failed. |
| `PACKAGE_NOT_PUBLISHED` | The package is not on the live installer. |
| `SITE BRANCH ONLY` | `gh-pages` moved. The live bytes have not been fetched. |
| `DOWNLOAD_HTTP_VERIFIED` | The fetched installer bytes match the qualified package. |
| `DEVICE NOT TESTED` | There is no attributable hardware session for these bytes. |
| `DEVICE VERIFIED` | Same-byte, matching-model owner hardware evidence exists. |
| `GIT DELIVERY PENDING` | The commit is only local, or the push was refused. |
| `NO_NEW_DELIVERABLE` | No eligible change. No empty commit and no empty pull request. |
| `DELIVERY_BLOCKED` | The required write was refused. |
| `EXACT-SOURCE VALIDATION PENDING` | The exact tree could not be reconstructed. |

Optional commit trailers, one line each, when they help the next reader:

```text
X0X37-Role: <role>
X0X37-Stage: <label>
X0X37-Consumer: <next owner>
X0X37-Evidence: <path or pull request>
X0X37-Next: <one action>
```

Do not add a trailer-only commit on an unchanged tree. `NO_NEW_DELIVERABLE` is a valid run result: cite the unchanged SHA and the reason. `DELIVERY_BLOCKED` names the refused operation.

## Consumer acceptance

A later run acts on a handoff only after it reads the remote head, the diff, and the claimed evidence. It runs the checks its own stage requires. It does not edit the producer branch. It writes its disposition on its own branch or as a pull-request review.

Deduplicate on `(repository, branch, full commit, evidence revision)`. A changed head invalidates affected reviews. Docs-only reuse of package bytes requires proof that every compiler input and pin is unchanged, plus a recorded mapping to the current root. A subtree SHA is not the root SHA.

If the exact tree cannot be reconstructed, do the narrower work and record `EXACT-SOURCE VALIDATION PENDING`. Do not report a build pass.

## Bounded run result

End with:

- the remote commit and pull request, or `GIT DELIVERY PENDING` with the refused operation
- checks executed and checks not run
- the stage that advanced and the stage that remains
- per model, when installability was in scope: `NEW VERIFIED LIVE`, `PRIOR VERIFIED LIVE RETAINED`, `NO INSTALLABLE YET`, or `REVOKED/UNSAFE`
- the next owner and action

Configuration notes, build results, HTTP results, and device results stay in separate sentences.
