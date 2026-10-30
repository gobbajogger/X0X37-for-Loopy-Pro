# Recovered local source reference — NOT integrated

This text-only archive preserves 81 unique source/test/tool blobs from original local commits `0e58b960faa25a3c43d342cd923f5f14425ab2ba` and `e2f881b0b0d5fbde8b4e352ab03c8ae2adc46058`. Original commit/tree identities were independently verified from local loose-object bytes; files are exact Git blob matches. The GitHub API reported no commit for either original SHA. This archive is a filtered projection: it does **not** preserve the original commit SHA or full history.

## Reuse, not replacement

Use [existing UI archive PR #703](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/703), [coverage PR #706](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/706), and [request PR #699](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/699). No production path is changed. Comparison of the 0e58b960 and e2f881b0 local trees against #703 found 622 and 649 matching source/test/tool entries respectively; comparison against one-build alone found 597 and 625. Of 133 remaining unique source candidates, direct GitHub blob reads found 52 already present; they were omitted. Only the remaining 81 NOT_FOUND blobs are included once each. MANIFEST.json retains all source commit/path associations where a blob was shared.

Historical files may omit newer fixes or use retired engine paths. Never restore them wholesale. The current source owner should compare only the required behavior and preserve current guards, source ownership, review budgets and test-suite union. This is source preservation, not feature acceptance, installability or delivery to a device.

## License and limits

The original repository [GPL-3.0 LICENSE](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/094bb1a55c159aefb0e2bfa39790a1b4b74cee40/LICENSE) applies; existing SPDX and copyright notices remain unchanged. The six retained header variants are public project headers at already-public paths with GPL-3.0-only SPDX, not private SDK headers. No private headers, SDK/toolchain binaries, credentials, raw captures or personal quotations are included.

Checks performed: exact Git blob and SHA-256 identity, UTF-8/text validation, credential/private-reference screening and Python AST parsing where applicable. These checks do not establish behavioral correctness. Source/runtime, Mac, golden, performance, full firmware/controller and hardware checks are **NOT_RUN**. Some fixtures depend on other historical source; this archive is not a standalone runnable build.
