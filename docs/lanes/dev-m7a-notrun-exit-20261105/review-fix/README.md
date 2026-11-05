# PR #854 review-fix qualification

Source candidate: `14413a784ff0eab30e55d4d8744f1f501d1e5a9f`.
Source tree: `967871135f3e2edcd27ee0637cf1c6b903348695`.
Parent: `de871c51fd3fdef0876e6a9d94d8ead4a13fd875`.
Base: `48e92b76c2f476cf40771e00a04e1838136bb55d` (`wip/one-build`).

The defaults mutant expected a comment absent from project.c. Its search now
matches the unique assignment. The original command exits 1; the repaired command
exits 0 and replaces only the reset assignment. See [the regression receipt](defaults-regression.json).

The first compiled mutation run also exposed a surviving mute_reads_as_off
mutant: a muted sequencer is silent under both implementations. The independence
test now checks that an ON, sequencer-muted part accepts and releases a live bass
note. This detects the incorrect coupling without changing firmware behavior.

`git rebase --rebase-merges` completed on the current base while retaining the
published ancestry and the same candidate HEAD. A normal fast-forward push is
possible. Only the two host-test files change executable source from the parent.

- [Full unfiltered host suite](host-suite.json): **340 PASS, 0 FAIL, 0 SKIP,
  0 NOT_RUN**, exit 0 in one complete invocation; strict preflight 16 PASS.
  [Complete terminal output](full-host.txt). The normal part-off checks also
  contain no FAIL or NOT_RUN cases.
- [Compiled mutation suite](mutants.json): 31 killed, 0 survived, 0 build failures;
  exit 0, with the required resolvable base. [Result lines](mutant-results.txt).
- [SDK V1.2.1 native builds](build.json): Elite DEV, Pro DEV and matching controller
  PASS; all 11 SDK input hashes and the pinned JieLi compiler hash match.
  [Native output](native-builds.txt).
- [Independent review](review.json): no findings on the exact two-file delta.

The actual builds ran in this Linux executor. Firmware, build tools, assets,
host simulator and controller inputs are byte-identical across the build and
fixed source candidates; only host-test source changes. Package hashes also
match the previous candidate. This is BUILD_ONLY proof. No device, installation,
publication or production result is claimed.

The initial restricted-sandbox host attempt was interrupted after Node reported
spawnSync python3 EPERM despite the child completing with status 0. A minimal
probe reproduced this executor issue and succeeded with explicit network access.
The complete host invocation was restarted with that access. It then exposed
missing Clang sanitizer runtime libraries in optimization-equivalence and
original-stale-release. The matching compiler-rt package was restored; both
complete selectors then passed with sanitizers enabled. A final full invocation
uses the complete [host compiler setup](host-compiler.json). No assertion,
selector or expected count was weakened. The earlier diagnostic mutation run is
superseded by the complete 31-mutant run on the fixed source.
