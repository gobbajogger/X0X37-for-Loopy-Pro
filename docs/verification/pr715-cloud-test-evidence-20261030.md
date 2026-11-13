# Free Play rate-pad regression: cloud verification checkpoint (30 October 2026)

**Repository:** `gobbajogger/X0X37-for-Loopy-Pro`
**Integration base:** `wip/one-build@cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`
**Proposed code HEAD before this evidence note:** `ccc073071c653295ad917f49ea7939408c6a0b8a`
**Current code tree before this note:** `d1f84cebbcc4b5bb90d21083331a5bf70ce9b87d`
**PR:** [#715](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/715)

## Executed exact-scope tests

The six original Miami physical Free Play pad functions were extracted from GitHub blobs `firmware/hal/smk_input.h@4baf29c88f38a08f4eeff3d8d963c776e2341ebb` and `firmware/src/app/ui.c@2e51cc1ffa6026c98eeaba76774b041982f5e7ac`; these identities are unchanged in this PR. The submitted test file is `tests/host/test_freeplay_rate_pad_port.py@50f3c477bbdfd7330a88b20cd99037b297e8995d`.

The already-checksummed compiled harness was rerun locally on Linux x86_64 with `-std=c11 -O1 -Wall -Wextra -Werror -Wno-unused-function -fsanitize=undefined -fno-sanitize-recover=all`. GCC and Clang both passed with `-DX0X_DEV_WOBBLE=0` and `-DX0X_DEV_WOBBLE=1`: **4 of 4 fresh executed runs passed**. The prior exact-source scoped regression archive records 19 passing commands, including independent AddressSanitizer/UBSan cases. This checks source-extracted handlers, **not a complete current firmware executable**.

## Pinned native inputs verified here

- User-supplied JieLi toolchain ZIP: SHA-256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`; Pi32v2 Clang 4.0.1 invoked successfully.
- User-supplied private build-input ZIP: SHA-256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`; `python3 kit.py verify` passed **all 11 SDK AC79NN V1.2.1 pins**.
- SDK V1.2.0 was **not** substituted.

## Complete source/full-test gate status

The GitHub Git Data API verified the PR tree and identified the transition from the local historical `6ae0928fe3264ee759c25c4f879eff4dc5dc3b20` snapshot: 222 added/modified blob objects and 216 deletions. All immutable remote tree metadata is readable. The 146 modified historical files were staged privately for an API-delta transfer; the complete exact PR tree has **not** been materialized as a local working tree.

The GitHub connector's `fetch` rejects `zipball` and `tarball` endpoints as unsupported URLs (`INVALID_ARGUMENT`). The native shell's direct GitHub API HTTPS check fails with `curl: (6) Could not resolve host: api.github.com`. Network Git was never used. These are connector-to-native byte-transfer limitations, **not missing SDK, compiler, GitHub read authorization or private input files**.

**Full host test suite at PR #715 head: NOT_RUN.**
**Native SDK V1.2.1 Elite firmware build at PR #715 head: NOT_RUN.**
**Native SDK V1.2.1 Pro firmware build at PR #715 head: NOT_RUN.**
**Exact-current bit-exact audio, full QA, and hardware checks: NOT_RUN.**

For environment diagnostics only, the older `6ae0928` complete source was built with the pinned JieLi compiler and SDK: historical Elite and Pro builds succeeded. Historical Elite and Pro packages were each 651,455 bytes; their respective SHA-256 values were `3bc6a7017a278e53217bf29014f895ebf12b96cf98c1aa93c94a164fc0823829` and `8da63e9457e3be7bcd5c3774245308faa90bda88976fae0aa55b40c98b2e24cc`. They were **not** attached, pushed, published, installed, or attributed to PR #715.

A fresh attempt at the historical `6ae0928` full host suite ran over ten minutes and was stopped after recording **221 results: 166 PASS, 50 FAIL, 5 NOT_RUN**; this is an **INTERRUPTED historical diagnostic**, not full suite completion, nor evidence about the current PR. Many failures were GCC weak-inline/attribute diagnostics; others need independent examination.

## Exact next action / qualification boundary

Use an approved byte-preserving bridge from the GitHub connector/API into the native runner to reconstruct and independently check the entire PR source and tree SHA. Then run the full host suite and both pinned Elite/Pro development builds against that exact source. Diagnose and fix current-source failures, rerun affected checks and perform independent review before asking the Merge Integrator to admit #715. No private #684 headers, toolchains, controller/firmware binaries, or other restricted bytes may be committed or pushed.

**State:** SOURCE TEST REGRESSION PUSHED; scoped host checks PASS; full PR build acceptance **INCOMPLETE**. No merge-ready or DEV CHECKED claim.
