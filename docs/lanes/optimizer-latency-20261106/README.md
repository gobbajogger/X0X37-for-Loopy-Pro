# Latency lane recut and validation

Retain the raw-key fast path and bounded arp expansion while validating the lane
against the requested one-build base. Players retain the same MIDI note sequence;
this recut changes no production source, audio arithmetic or persistent RAM.

Pinned base: `48e92b76c2f476cf40771e00a04e1838136bb55d`.
Original lane: `8081c00d2552f0d5ba42e4e27485d54ff25f9066`.
Both latency optimizations already reached the base through
`a49605bcb34c8d6751817dd33699640d19a24c29`. Their function bodies and the test
runner are byte-identical. The test source differs only in its baseline-reference
comment; both referenced baseline functions are also identical.

Preserve the complete pinned-base tree and add only these evidence files. Ordered
merge parents are the pinned base followed by the original lane. This is an
ancestry-preserving recut that permits a normal fast-forward lane update without
importing the unrelated source changes from the old lane. It is not a rebase.

## Host proof executed here

Final coverage: **336 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN**.
The root Codex thread ran the entire host suite here: 335 passed and one failed
because the optimization matrix required a missing Clang executable. After
installing Clang 19.1.7 and its sanitizer runtime in the workspace, the root thread
reran the complete optimization selector successfully. A final log audit also
found three optional Clang variants skipped inside the initially passing
original-stale-release selector. The root thread reran that complete selector:
all five GCC/Clang plain and sanitizer configurations passed 164 checks each.
Source and test bytes did not change. This reuses the other 334 passes; it does
not claim one green unfiltered invocation. [host-suite.json](host-suite.json) preserves both outcomes,
commands, compiler/reference identities, selector coverage and log digests.

The latency check passed 20,459,520 exhaustive key cases and 414,720 arp cases.
The full optimization rerun also passed GCC/Clang optimized and ASan/UBSan decoder
checks and the 909 state/PCM comparisons. Historical 909/808 reference commits
were recovered from existing forks and verified at their original full SHAs.

## Native proof executed here

Both Elite and Pro DEV builds passed on Linux x86_64 using JieLi Clang 4.0.1 and
SDK V1.2.1, tag `AC79NN_SDK_V1.2.1_2023-12-13`, commit
`d179b4484759423312073f5fbb232501aa491047`. All 11 consumed SDK files matched
their pins. The matching controller build also passed.

| Metric | Elite | Pro |
| --- | ---: | ---: |
| App bytes | 480568 | 480568 |
| Package bytes | 651457 | 651457 |
| App code headroom | 72104 | 72104 |
| App RAM headroom | 2524 | 2524 |
| Pool headroom | 8600 | 8600 |

The controller is 61088 bytes, CRC32 `9aa7f77f`, with 26336 of 26368 RAM bytes
used. App/package hashes reproduce the pinned optimization-stack measurements.
[native-builds.json](native-builds.json) records exact hashes, budgets, SDK pins
and log digests. [source-closure.json](source-closure.json) identifies the unchanged
build/test input scope. These are BUILD_ONLY results. Target cycles, hardware,
installation, release and publication are not measured or qualified here.

## Review and delivery

An independent read-only review found no remaining latency delta or source issue;
see [source-review.json](source-review.json). The reviewer ran no builds or tests.
Final delivery must retain the complete base tree and change only this evidence
directory. Keep one PR into `wip/one-build`. No protected-ref write, `.github`
change, firmware/SDK/private-header addition, credential, or device action belongs
to this recut. Generated binaries stay local and are not delivered.

## Independent successor QA

An independent Codex QA thread tested PR head
`072c1bdfe626c2436600b9900d0c238458cdab42` against the unchanged pinned base.
All 4,935 pre-existing tracked base blobs matched the working checkout, and the
PR delta remained exactly these five evidence files. No recut was needed.

Elite and Pro DEV native builds and the matching controller passed with SDK
V1.2.1 (all 11 pins verified) and the pinned JieLi compiler. All app, loader,
package and controller hashes reproduce the producer records. Package model,
CRC and budget checks passed.

The independent full host run passed **336 PASS, 0 FAIL, 0 SKIP, 0 NOT_RUN in one
clean unfiltered invocation**, with strict preflight 16/0. All internal Clang
variants ran. This supersedes the need to rely on the earlier composite coverage
for QA readiness; the earlier execution history remains recorded above.
An earlier sandboxed attempt was aborted after a reproduced Node subprocess
EPERM; the approved execution mode resolved that environment error without
source or test edits. See the `independent_qa` records in the JSON evidence.

This follow-up changes evidence only. Tested build and host inputs remain
byte-identical after the evidence update. BUILD_ONLY does not qualify hardware,
installation, publication or production promotion.
