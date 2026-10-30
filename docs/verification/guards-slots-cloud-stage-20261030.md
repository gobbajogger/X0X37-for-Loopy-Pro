# Guards/slots exact-source host validation checkpoint (30 October 2026)

**State: INCOMPLETE.** This is a source-and-test checkpoint, not an exact-HEAD full-suite or firmware-build PASS.

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`; existing draft PR [#708](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/708).
- Isolated branch: `chat/guards-slots`; stage input head: `bb8f0d262feace536f63a5e6f190ff476250d69d`.
- **Base**: `wip/one-build` at `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`.
- Scope: Chicago/Miami original engines, stable 303 slots, project revision 13 only. No main/nightly merge, firmware publish or device operation.

## This stage's executed checks

| Check | Result | Exact scope |
| --- | --- | --- |
| Host builder build-selection fixtures | **PASS, 10/10**, exit 0 | Exact GitHub `host/build_host.sh` blob `e81cff17569e0912f6d10043d9efeb23682d7e1b`. Asset creation, default rate pads, optional disable, retired-engine rejection and error propagation |
| Drum909 test syntax | **PASS**, GCC `-fsyntax-only`, exit 0 | Exact file blob `ae1e6d41b131834069b918b2fdf4fc17db2ab453`, reused from existing `fix/one-build-host-test-failures`. Warning lines from existing firmware headers were emitted; no compiler errors |
| Live GitHub branch and PR | **PASS** | Base verified; no force-push; source-only change paths |
| SDK/toolchain provenance | **VERIFIED INPUT** | Local pinned SDK V1.2.1 kit and JieLi compiler, not a current-HEAD firmware build |
| Full host suite on exact PR head | **NOT_RUN**, pass/fail/skip **unknown** | The owner-reported 69 failures on the exact base are a separate historical baseline, not a rerun |
| Current-HEAD Elite DEV | **NOT_BUILT** | Requires exact source tree |
| Current-HEAD Pro DEV | **NOT_BUILT** | Requires exact source tree |
| Goldens / Valgrind / hardware | **NOT_RUN** | No source-matched measurements |

Native GCC syntax evidence was obtained by applying the one-line change to a recovered local `tests/host/drum909_test.c`: the file's calculated Git blob SHA exactly matched the GitHub head file. Its include graph was historical; this is a **scoped syntax check**, not whole-HEAD compilation.

## Source-transfer diagnostics

1. Connected GitHub API request to `/repos/gobbajogger/X0X37-for-Loopy-Pro/zipball/c744f8641106cccd7993f115183d6d6eab75d857` returned:
   `INVALID_ARGUMENT: HTTPError: 400: GitHub Fetch URL is not an allowed public GitHub repository or search endpoint.`
2. The required fallback `/git/trees/<head>?recursive=1` succeeded and was **not truncated**: 4,582 entries, 4,223 blobs at the input head.
3. Native GitHub HTTPS transport returned `curl: (6) Could not resolve host: api.github.com`; no network Git was used.
4. Against a verified historical 6ae local bundle, the input tree had **240 changed/missing blobs** and **217 obsolete local paths**, about **2.8 MB** of changed blob content. The additional drum909 test commit makes 241 changed/missing blobs. The connector can read individual UTF-8 blobs, but does not materialize the ZIP or an entire current checkout into the Linux runner.
5. A full-suite run started on the historical tree, then was deliberately stopped because the source was **not** the PR head. It contributes no current-head test count.

## Reuse and ownership

The maintained feature registry exists in the separate `docs/feature-registry` branch at `c7ca98ae21e6bff548373fe87cad58582ac739a7`; `docs/FEATURE-REGISTRY.md` is absent from `wip/one-build` and this PR's source tree. Its guards/slots row points to PR #708 and the base-fix lane. The one-line drum909 repair was reused, not independently reinvented. PR #721 holds the owner's private-header inputs on its own branch; no headers or binaries are committed here.

## Next acceptance action

Reconstruct the exact current source with all blob SHAs and allowed private inputs, then run `X0X_ORIGINAL_SYNTH=1 X0X_WOBBLE=0 X0X_TEST_JOBS=4 sh tests/run_tests.sh` to a measured zero-fail/zero-skip result. Reproduce and repair remaining failures, then run SDK V1.2.1 Elite and Pro target builds on that same source. This stage remains open pending those measurements.
