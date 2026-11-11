# PR #712 cloud build and source-transfer preflight, October 30, 2026

**Result:** PARTIAL / NOT QUALIFIED. No full current-candidate host suite, golden PCM or Elite/Pro firmware build has completed. The archive-source builds below must **not** be attributed to the PR.

## Exact GitHub candidate

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`
- Source branch: `wip/engines-ui-one-build`
- Inspected HEAD: `82b3125261e14000454f43c0d1974f09e62c79ea`
- Source tree: `c2eff03ae01e26b761e17fa44c5c686f49d90f4c`
- Verified PR #712 base: `wip/one-build` at `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`
- No direct changes to sound source, device firmware or tests in this preflight.

## Transfer attempts

1. Authenticated GitHub connector ZIP: `GET /repos/gobbajogger/X0X37-for-Loopy-Pro/zipball/82b3125261e14000454f43c0d1974f09e62c79ea` returned:
   `INVALID_ARGUMENT: Error code: INVALID_ARGUMENT; Error: HTTPError: 400: GitHub Fetch URL is not an allowed public GitHub repository or search endpoint. (Response: None)`
2. Required fallback `GET /git/trees/<head>?recursive=1` **PASS**: 4,610 entries, 4,250 blobs, `truncated=false`, 23,344,920 declared blob bytes.
3. Offline project-source archive (October 28 original source) hash verified, raw Git tree manifest has 4,163 blobs. Compared by exact Git blob SHA: **3,925 match** the PR tree and **325 do not**. Changed/new blobs total 3,278,487 declared bytes; all are UTF-8/text-like paths, not protected binary inputs.
4. Authenticated `GET /git/blobs/<sha>` API in bounded batches **PASS for 10/10 first changed blobs**, Git-reported sizes matched returned UTF-8 byte counts. Larger batched calls returned `Code Mode exceeded the maximum number of tool calls.` This API response cannot be written to the native container by the GitHub connector. The complete changed-blob delta was therefore **not materialized into the local checkout**. Calling the old archive an exact checkout would be false.
5. Direct container HTTPS to `api.github.com` and `codeload.github.com` failed with `NameResolutionError: [Errno -3] Temporary failure in name resolution`; no `git clone/fetch/pull/push` network commands were used.

## Native build input verification

- Toolchain ZIP SHA-256 `ed379c483468c44f10cf3be7db32356b9623e16320a021cf2a4e06ec96ab5f19`: exact, 44 symlinks preserved, JieLi clang 4.0.1 runnable on Linux x86-64.
- Private input ZIP SHA-256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`: `python kit.py verify` **PASS 11/11 exact SDK V1.2.1 pinned files**.
- **Do not substitute SDK V1.2.0.** These files remain outside public Git.

## Cloud build smoke, OFFLINE OCTOBER 28 SOURCE ONLY

Original local source: `a7082231e4b134c24384bc159864a813b93a17ea` (the archive's recorded, non-GitHub SHA). `original-source.tar` SHA-256 `204343a70250801346147c063b24fdddc46726badebbb3ed8b50c85e5e5a560c`. This code predates the PR, so **neither build is for current HEAD**.

Commands, with `JIELI_TOOLCHAIN` pointing to the verified extracted compiler and `AC79_SDK` to `sdk-pinned-inputs`:

```sh
X0X_ORIGINAL_SYNTH=1 X0X_WOBBLE=0 X0X_DEV_WOBBLE=1 BOARD=smk37 MODEL=elite sh build.sh
X0X_ORIGINAL_SYNTH=1 X0X_WOBBLE=0 X0X_DEV_WOBBLE=1 BOARD=smk37 MODEL=pro sh build.sh
```

| Historical input | Exit | Package bytes | Package SHA-256 | Free code bytes | RAM in-use / max | Free pool bytes |
|---|---:|---:|---|---:|---|---:|
| Elite | 0, PASS | 651455 | `42ffd879be7a690c2f6511b875ea6991fe25db3f75d46646ea29fd34348532af` | 195388 | 95144 / 98304 | 15464 |
| Pro | 0, PASS | 651455 | `5d8ed2f5ee5bd9bd9f953aade93066ed9eff3ab909fa704ac2039a620251867e` | 195404 | 95144 / 98304 | 15464 |

These hashes are reproducibility receipts only, **not** permission to publish/flash. Local build log hashes:
Elite `7c9690f842ddf5f051f2028d2f6d0f61a9b08ff075b9660617145230f42bbfc2`;
Pro `5170a986f28adf727c4249d0d3cb82b630434f8f06b8920813424c332db9ebf3`.

## Host suite smoke, OFFLINE OCTOBER 28 SOURCE ONLY

Command: `X0X_TEST_JOBS=4 X0X_TEST_TIME_SCALE=20 timeout 150 sh tests/run_tests.sh`.
Exit **124 (timeout)**, so the suite is **INCOMPLETE/FAIL (timeout)**. Before cutoff, the script recorded **53 PASS, 13 FAIL, 0 SKIP** among **66 completed test result records**. Failures were `fastmath`, `fastmath-ubsan`, `bass303-sustain`, `project-303-stream`, `wave-motion-migrate`, `project-protocol`, `project-save-preflight`, `editor-ota-dispatch`, `storage-scratch`, `project-stage`, `project-runtime`, `smc-queue`, `smc-solo-overflow`. Several errors were GCC 14 warnings promoted to errors; `storage-scratch` required a Git repository that was absent in this extracted TAR. Do **not** apply or relabel these as PR #712 failures.

Prior source-scoped exact-Git-blob regressions for Chicago clock, Chicago chorus and Miami helpers are reusable because the checked 8 Git blob hashes still match current HEAD. They are **not** the current whole-application test/build gate.

## Current candidate qualification and next action

- **Exact PR HEAD full suite:** NOT_RUN, PASS=0 / FAIL=0 / SKIP=not applicable (source materialization incomplete).
- **Exact PR HEAD Elite build:** NOT_RUN, no package or ELF.
- **Exact PR HEAD Pro build:** NOT_RUN, no package or ELF.
- **Exact PR HEAD golden PCM / active and idle CPU / DEVICE:** NOT_RUN.

**Next:** the source-transfer/executor owner completes an exact SHA-verified 325-blob delta transfer or supplies a directly materializable GitHub source archive, preserving unchanged 3,925 Git-identical files. Then run `tests/run_tests.sh` to completion, fix candidate-specific failures, run both model builds against the full tree with SDK V1.2.1, verify memory/package/controller and original bit-exact golden results. Keep PR draft; no merges, releases or installs.

No credentials, private headers, restricted SDK/toolchain files, firmware images, samples or raw captures are committed.

## Later integrated-base update

After this preflight, `wip/one-build` advanced from `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21` to `b982c1eebc2ef731e0379103506aecb3df0b85e5`. GitHub comparison shows 31 incoming commits changing **only 20 documentation/routing/registry paths**, with **zero overlap** against PR #712's 55 changed paths; no changed firmware source, tests, SDK pins, or target bytes. The current branch takes that exact new base as an additional parent in its next source-preserving documentation merge. The measured archive smoke and transfer attempts still refer to their original inputs; no current-head firmware build is implied.
