# Independent QA for PR #853

Panic could leave a live bass recording active after silencing its note. The
recorder then overwrote future pattern cells as TIEs and suppressed playback.
Panic now finalizes every bass recording and invalidates its ownership before
releasing notes. Playback continues and REC remains armed for a new press.

The branch already contained the current `wip/one-build` tip
`48e92b76c2f476cf40771e00a04e1838136bb55d`; the initial compare was
3 commits ahead and 0 behind. No rebase or history rewrite was necessary.
Starting PR head: `12c98da192df97d1bbcd2b3b6f19302b8b61f662`.
The complete tested source tree, before adding these two evidence files, is
`15a119f6ce163435d11f3d9c278637d202498749`.

## Verification

- The new recording regression failed 44 of 142 assertions on unchanged source,
  including future-cell corruption and suppressed normal playback. The fix passed
  all 142 assertions in normal and ASan/UBSan runs. It covers all four input sources
  on both bass voices, delayed releases, ownership invalidation and new recording.
- Independent read-only review found no blocking issues. That reviewer also ran
  the frozen regression: 142/142 normal and 142/142 ASan/UBSan, exit 0.
- The complete host suite passed in one qualification run: **341 PASS, 0 FAIL,
  0 SKIP**, exit 0. Strict preflight passed all 16 checks. GCC 14.2.0 and Clang
  19.1.7 were available; sanitizers and pinned DSP comparisons ran. All six panic
  mutants were killed. The runner restored its temporary source mutations.
- Elite and Pro native DEV builds passed with SDK V1.2.1 commit
  `d179b4484759423312073f5fbb232501aa491047`. All eleven SDK input hashes were
  enforced. Both applications are 481,656 bytes, with 71,016 bytes of code space
  free. Static RAM is 95,780 / 98,304 bytes; pool free is 8,600 bytes, above 8,192.
- The matching controller passed: 61,088 bytes, CRC32 `9aa7f77f`. Both model
  packages and locally composed controller packages passed model/integrity checks;
  the embedded controller bytes equal the independently built controller.

`independent-qa.json` contains source/input pins, all 341 check names and log
hashes, artifact hashes, budgets and the regression receipt. These are BUILD_ONLY
checks, with no formal DEV CHECKED record, hardware or publication claim. Binary
outputs and vendor inputs were not committed or distributed.

## Classic Build handoff

The LoopyX0X37 Classic Build handoff policy is active for this task. Root retained
source ownership; the independent reviewer consumed the frozen candidate and
returned actual test results. The existing integrator owns the next review and
integration action through **PR #853 into `wip/one-build` only**. Its consumption
is pending; this QA task does not claim that integration has occurred.

No blockers remain for this source QA outcome. No protected branch, `.github/`,
flash, USRTRIM, bootloader, credential or raw-capture change was made. No new PR,
merge, schedule or automatic successor was created.
