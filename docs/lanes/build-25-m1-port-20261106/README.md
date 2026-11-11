# Build-25 M1 continuation

The M1 panic lane is prepared against `wip/one-build` at
`48e92b76c2f476cf40771e00a04e1838136bb55d`. Players retain the panic
chords that release stuck notes while playback continues, together with the
current base optimizations.

The original published lane tip is
`4a69f6e74454342e204dba3cd74cae84299a03dc`. The local rebased source
commit is `708a25a4ad61a883e66b9b59f228accefbb5acda`, complete root tree
`15570c4a31663374f3b93d5abfefed7843c9d494`.

Only `tests/run_tests.sh` conflicted. The resolution retains the base's
`optimization-equivalence` generated-header dependency and both panic dependencies.
The firmware optimization hunks remain intact. No functional source repair was
needed after the rebase.

## Root-run proof

- Elite and Pro native development builds: PASS, exit 0, SDK V1.2.1 at
  `d179b4484759423312073f5fbb232501aa491047`; `X0X_DEV_PADCAL=1`.
- Both applications: 481,592 bytes, 71,080 bytes below the code limit.
  Static RAM: 95,780 / 98,304 bytes, including alignment. Pool free: 8,600 bytes;
  required minimum: 8,192 bytes. `.ram_text`: 2,712 bytes.
- Matching native controller: PASS, exit 0, 61,088 bytes, CRC32 `9aa7f77f`.
  Both composed model packages preserve the model identity, pass CRC checks,
  and contain exactly the built controller bytes.
- Full host runner: all 341 checks executed here by root. Strict preflight:
  16 PASS. Initial result: 340 PASS, 1 FAIL, 0 SKIP, runner exit 1.
- The sole failure was a missing required host Clang in `optimization-equivalence`.
  Verified Debian Clang 19.1.7 and its sanitizer runtimes were extracted under
  the task workspace. Root reran that complete closure on an identical source
  checkout: PASS, exit 0, including GCC and Clang optimized and ASan/UBSan runs.
  Other unchanged full-suite PASS results remain valid.
- Final effective result: **341 PASS, 0 FAIL, 0 SKIP**. This is a full execution
  plus its documented repair rerun, not a claim that the initial runner exited 0.
- Panic chord, note clear, real keyboard/SMC routes, and all six runtime mutants:
  PASS. The sequencer's pinned TB-3PO comparison and actual package checks ran.
- Independent static source review: no blocking rebase findings. This does not
  claim independent executable QA or device proof.

The first sandbox-bound attempt was stopped after ptrace and subprocess
restrictions. The complete run and repair rerun used approved execution outside
that shell sandbox. No sanitizer, assertion, scenario or comparison was disabled.
The native generation cache was directed into the task workspace.

[validation.json](validation.json) records source/input pins, artifact and log
hashes, budgets, all check outcomes, and the original failure and passing repair.
Binary artifacts and restricted SDK/toolchain inputs stay outside the source delta.

## Delivery and next action

Delivery reconstructs the rebased source tree through the connected account's
default Git identity, verifies that complete tree against the tested tree above,
and preserves the old published tip as an additional ancestor in the evidence
commit. This permits a normal fast-forward update of the existing lane, without
force pushing. The evidence commit adds only this directory to the tested tree.
The PR targets `wip/one-build`; the assigned integrator owns its next review and
integration action. This thread does not merge the PR.

Earlier panic reports describe earlier runs, including historical NOT_RUN states.
The current implementation has one keyboard chord state and separate SMC peer
states. This receipt records the current continuation's proof.

State: **BUILD_ONLY**. Hardware, publication, production qualification and
independent executable QA remain NOT_RUN. No protected branch or `.github/`
change, device operation, binary distribution or credential change is included.
