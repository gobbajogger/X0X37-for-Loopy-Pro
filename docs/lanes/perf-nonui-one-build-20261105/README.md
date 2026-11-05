# perf-nonui on the current one-build base

A USB cable retry used to block the foreground loop for 25 ms. The lane keeps the reset settling interval while letting foreground input continue, so reconnect attempts do not pause playing controls. Boot bring-up keeps its original blocking interval.

Current owner request authorizes rebasing onto `48e92b76c2f476cf40771e00a04e1838136bb55d` and supersedes the historical a06-only checkpoint. The lane was rebased locally from `c71695452e04f4e7618562dbb80c2f54865c1f79`. The base already contains undo/project/bus-FX source changes, so the remaining firmware change is USB retry. Test registrations retain both base and lane coverage. Delivery preserves the previous lane ancestry for a nonforced branch update.

Checked source: `4822094c39198da5d824b1ec43ac741830d9d30b`, tree `e6d07a39e2a1677d94c3b54551609abf77177b9d`. Final delivery adds only this evidence directory to those checked bytes. The connected GitHub account supplies the delivery commit's default author and committer.

Full host coverage: **340 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN**. The initial full run returned 339 PASS / 1 FAIL because host Clang was missing. After verified Clang setup, both optimization-equivalence and the stale-release check (including its previously skipped Clang variants) passed. The initial and repair receipts remain separate; the final summary combines exact-source proof without rewriting the failed receipt.

Elite and Pro DEV builds both pass with SDK V1.2.1. Each app is 480664 B. RAM allocation is 95780/98304 B; pool is 335464/344064 B (8600 B free). `.ram_text` is 2712 B, 844 instructions, no calls. The matching controller build passes. All saved native artifact digests still match after the suite. See native-metrics.json for model/package/controller hashes and inputs.json for dependency pins.

Independent read-only review found no blockers. This is BUILD_ONLY evidence, without installability, release, publication, hardware or measured target-cycle claims. No binary or private input is included. Next owner: the successor thread verifies the delivered PR and supplies its readiness packet to the existing integrator; it must not merge or touch protected refs.
