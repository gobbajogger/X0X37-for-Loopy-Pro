# USB retry and 909 recut onto one-build

This recuts the remaining lane changes onto
`48e92b76c2f476cf40771e00a04e1838136bb55d` from `wip/one-build`.
The base already contains the earlier optimization stack. The new delta carries
`32802577` (909 packed tanh pair lookup), `c693fd6c` (cooperative USB retry), and
`74bd41b74` (detach during the settle window).

The USB retry leaves the main loop available during the 25 ms reset settle.
Boot still uses its existing blocking wait. The 909 change retrieves adjacent
tanh values with one packed-table search while preserving exact values.
No target timing or hardware result is claimed.

The base's feature policy is retained. Three mock edits for UAC-output tests
absent from this base were excluded; no USB-output feature history was imported.
The only further source edit labels the retry test clock as simulated.

## Verification

The root Codex session personally executed every build and test. An independent
read-only Codex subagent reviewed the source and the recut selection; it ran no
tests or builds. Its comment-wording finding is resolved, with no remaining
functional findings.

Both Elite and Pro DEV builds use JieLi Clang 4.0.1 and SDK V1.2.1 commit
`d179b4484759423312073f5fbb232501aa491047`; all 11 consumed SDK pins match.
Each app is 480824 B, with 71848 B of code headroom, RAM occupancy 95780/98304 B,
and pool headroom 8600 B (minimum 8192 B). The matching controller is 61088 B,
with RAM occupancy 26336/26368 B. The inherited initializer-braces warning is
present; both builds exit zero.

The extra 909 check compares 23280 blocks at both -O0 and -O2, including the full
7648-byte state and four PCM buses. Its exhaustive tanh test covers 1024 indices
and all 4278190082 non-NaN binary32 inputs at -O2. These checks pass separately
from the full-suite runner. GCC and Clang optimized and ASan/UBSan IMA checks pass.

The full unfiltered host suite passed in one fresh run: **337 PASS,
0 FAIL, 0 SKIP, 0 NOT_RUN**. The full selector matrix is in `host-suite.json`. `verification.json` records source/input identities, native metrics,
local artifact hashes, commands, review and exact exits. Historical lane receipts
are not used as fresh results.

## Delivery

PR: <https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/861>.
The normal reconciliation commit retains the original lane head as its first
parent and the pinned base as its second parent, with the clean recut tree.
This preserves the existing PR and lane ancestry without a force push.

Source and sanitized evidence only. No changes to `main`, `nightly` or `.github/`;
no binaries, SDK/toolchain bytes, tokens, release, merge, install or device action.
This is BUILD_ONLY evidence, not a DEV CHECKED, release or hardware qualification.
