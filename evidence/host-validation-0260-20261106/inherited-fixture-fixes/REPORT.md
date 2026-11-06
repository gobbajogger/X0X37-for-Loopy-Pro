# Two inherited fixture repairs

Original firmware candidate tree: 0260cd3f78a1b3717ef988d54aca767d65b6a2bb.
Previous 21-selector fixture repair tree: e3bd02f7a4aece4ce96aa0602fb41b990275b629.
Final test-only tree: d01ce34282e755aa7f332a0ed70f7241a0ad45b5.

Both failures were reproduced before editing, exit 1 each. Both are inherited fixture failures, previously reproduced on c295af1554f9e80a29864cd013995eccad66cd2c baseline. The incremental patch changes only tests/host/smc_fx_peer_view_test.c and tests/host/smc_ui_negative_test.py; no production firmware changes. Independent read-only review accepted the exact patches. See REVIEW.md.

Peer fixture: declare the real popup cache state required by extracted production functions; retain all 17 original assertions and add two assertions that nonzero cache sentinels clear at begin and timeout/off. The repaired direct selector exits 0 with 19 assertions. Both temporary reset-removal mutants compile and fail their intended new assertions (exit 1 each).

UI negative fixture: replace the obsolete cv_commit(3, 0, KNOB_Y) mutation anchor with current cv_commit_popup_knobs. Preserve all 14 mutants, the same injected cryptic footer, the unique-anchor guard, and the requirement for runtime assertion failure rather than compilation failure. Runtime result is recorded in results.json and logs.

The 21 previously repaired selector passes and three dependency-recovered selector passes are reused; no unrelated tests were rerun. The original full host suite exited 1; this packet is targeted repair evidence, not a new full-suite PASS. Separate legacy smc_peer_names fixture failure and TB-3PO external/native/package skips are outside these two repairs and remain distinct. Independent integration review is still required. No merge, firmware publication, native/device validation or release claim.

Apply inherited-fixtures.patch after the previous test-only patch (base e3bd02f7). Alternatively all-test-fixtures.patch applies directly to original0260 and yields the same final tree. manifest.json records both SHA256 hashes and changed files. Logs contain only repository technical test output; no private inputs or secrets were included.
