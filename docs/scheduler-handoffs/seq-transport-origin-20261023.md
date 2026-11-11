# Sequencer Clock — same-block MIDI transport origin guard

**Role:** Sequencer Clock (source-only); **status:** FOCUSED NATIVE HOST CHECKED / EXACT-TREE QA PENDING / DEVICE NOT TESTED / NOT INSTALLABLE.

**Root:** `seq_midi_rt(0xFA/0xFB)` marks `req_rt=1`; a subsequent local `seq_start()/seq_continue()` replaced the transport request but retained that remote-origin bit. `seq_stop()` canceled pending requests without clearing their remote origin. At `seq_advance`, this tags a local PLAY as remote (`start_local=0`). An immediate host MIDI Start echo then bypasses MID-14's local-PLAY deduplication, restarting the first beat twice.

**Reproduction:** Original source window from PR #86 `b680720d4031f73fe22143edaf1c6c55208f886e`, `sequencer.c` blob `6f0484488d64661b24ec64dbe36fed96fd08bfdf`. On native Linux x86_64, `python3 tests/host/seq_transport_origin_test.py --source fixtures/seq_transport_excerpt_before.c --cc gcc` compiled exit 0, test **exit 1, 7/16 failed**. Real C `seq_start`, `seq_stop`, `seq_continue`, `seq_midi_rt` plus the literal transport-application block are extracted into a focused native harness.

**Repair:** Add two origin-bearing queue helpers that publish the matching request type last, use them for local vs incoming START/CONTINUE, and clear the canceled tag on STOP. Retain PR #86 last-request-wins semantics, MIDI clock, Note Off, INTERNAL clock policy and clip-mode `midi_req`.

**Native results:** Same extracted production source windows after patch: GCC **16/16 exit 0**, GCC UBSan **16/16 exit 0**, Clang ASan+UBSan **16/16 exit 0**. Test script local Git blob `385b3b3d650c198567136973b79588b17f8a302d` and SHA-256 `6f93ee6cd1298f451ffc15d60bafb0414f177541a4b07eb53036e52c724c1214`; verify the committed blob matches exactly. This is source-extracted **not** whole-tree compiler, Elite/Pro model, controller or hardware evidence.

**Stack:** Depends on draft PR #86 at `b680720d4031f73fe22143edaf1c6c55208f886e` (last Start/Continue wins). This draft's PR target is the exact producer branch, not firmware `nightly`. No other ownership touched.

**Next:** Independent QA re-fetches this PR's immutable new full SHA and runs `python3 tests/host/seq_transport_origin_test.py --source firmware/src/seq/sequencer.c`, `X0X_TEST_JOBS=1 sh tests/run_tests.sh sequencer clk-lock transport-coalesce transport-origin` against **complete exact PR tree** and compares outstanding clock/seq PRs #35/#41/#48/#55/#65/#72/#80/#86. Sole Safe Merge reconciles source only after independent QA; Hourly Build separately qualifies matching pinned Elite/Pro+controller+backup/license/host gates. No main/nightly/Pages/flash/publish.
