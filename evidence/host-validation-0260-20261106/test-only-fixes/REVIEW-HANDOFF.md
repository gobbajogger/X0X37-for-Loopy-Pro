# Test-only GLOBAL fixture repair — review handoff

Base tree: `0260cd3f78a1b3717ef988d54aca767d65b6a2bb`.
Assembled test-only tree: `e3bd02f7a4aece4ce96aa0602fb41b990275b629`.
Incremental patch SHA256: `3d276f1ff923d9595b79a709c0e4565793ab301fc7ee020d4ac5d9ab7d35fb5f`.

Three isolated workers reproduced all 21 failing selectors before editing. Five menu/source-shape checks then PASS (exit0); eight storage/performance selectors PASS (exit0); six MIDI/clock scenarios PASS on their first corrected run, with the two arrow scenarios corrected further and passing scoped runs. The final clip-arrow run restores an immediate functional assertion and passes alone. Exact commands, intermediate failures and exit codes are retained in combined-manifest.json and group records. Passing unchanged selectors were not repeatedly rerun.

20 test files change; production firmware, host code, tools and runner remain byte-identical. The three patches touch disjoint files. Assembled file bytes match the latest tested group files. All original scenario functional expect lines are retained with multiplicity; only local listsel and arrowsteps UI-navigation expectations change. Menu C tests now enter visible semantic PROJECT/MIDI rows before asserting actions/channel order. The calibration/arrow source checks target the current guarded section/rearm behavior. Scenario category navigation supports the presence/absence of BLUETOOTH. Migration-rejection, storage-write, probability note counts, transport, confirmation, cancellation and performance assertions remain.

The full combined-tree suite was not repeated. This is reuse of exact unchanged production/helper inputs plus affected test results, not a new full-suite green run or independent approval. Existing exact c295af baseline receipts PASS for all21 and are retained in the parent evidence packet; updated fixtures target current menu behavior and are not represented as backwards-compatible flat-menu fixtures.

Inherited failing selectors: smc-fx-peer-view (missing extracted popup_knob_full_hash), smc-ui-negative (absent mutation anchor). Environment failure: optimization-equivalence stops at missing Clang. External drum808/drum909 reference selectors stay NOT_RUN; TB-3PO external subcheck and native package checks remain separate skips. Extra legacy smc-peer-names failure is unchanged and separately reported. No production fix, compiler workaround, native build, device, merge or release claim.

Reviewer focus: verify intended new GLOBAL back/entry semantics and arrow rearming, confirm every removed navigation assertion is legitimately obsolete, verify static source checks retain pad-calibration/ownership guards, inspect all21 exact-input red/green receipts, and preserve all inherited failures/unrun gates. Return to existing integration owner; independent review PENDING.
