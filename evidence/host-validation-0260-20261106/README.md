# Combined candidate host validation

**FAIL, runner exit 1: 335 PASS, 24 FAIL, 2 NOT_RUN.** Exact candidate tree `0260cd3f78a1b3717ef988d54aca767d65b6a2bb`, reconstructed from v6 packet `66cfa3d0724a039f29803c7770ae0d6233f7fada` and RAM patch `d5c328862929b14a49104011a843115a7f42d27e` on source base `c295af1554f9e80a29864cd013995eccad66cd2c`. Source/index remained exact after testing.

Full unfiltered command: `X0X_TEST_JOBS=8 sh tests/run_tests.sh`. Full logs and all 361 selector outcomes are retained. All 58 scenarios ran. Baseline comparisons reused the same compiler/runtime and only failed candidate selectors were selected. No passing tests were rerun during the follow-up diagnosis.

See [all 24 failures and first errors](FAILURES.md), [diagnosis and minimal repairs](DIAGNOSIS.md), [exact exits](exit-codes.json), and input/source manifests. 21 failures PASS on matching baseline; read-only parallel diagnosis traces them to obsolete flat-GLOBAL navigation/source-shape assumptions. Downstream assertions remain unproven until minimal fixture repairs are tested. Do not mask failures or change expected functional outcomes.

Inherited: smc-fx-peer-view missing extracted declaration; smc-ui-negative absent mutation anchor. Environment failure: optimization-equivalence reaches absent clang after passing GCC checks. All remain FAIL. Extra legacy peer-name script exits 1 on base/v6/candidate, separately from the 24 runner failures.

PASS: host build; original UI 23176 normal/sanitized checks and 40 PNG pairs; theme 14855797 checks; settings/peer-name checks; panic/mute safety checks with six killed mutants each; historical part-OFF all-on identity and cross-firmware storage; CLI executable-mode tests; 2646 sanitizer name-equivalence cases. Candidate/v6 audio: 17 scenarios / 50 audio and state hashes match. Base/v6 differs only in three solo-mute output files; no whole-base sound-identity claim.

NOT_RUN selectors: drum808/drum909 external reference checkouts absent. Sequencer local selector passed but external TB-3PO comparison was skipped. Native SMK package block and conditional prebuilt-package checks lack their app/loader/SDK/artifact inputs. Clang portion and subsequent optimization checks were not run after dependency failure. Native fit/build/packages, device and release qualification are outside this host-only assignment and unclaimed.

The original runner records zero/nonzero child status, not each failing numeric child exit. Saved full/selected-runner and supplemental direct-command numeric exits are exact; unknown child exits are not invented.

Evidence delivery is a later explicitly requested stage: text-only sanitized logs and records on an isolated branch; no firmware/source fixes, binary/private inputs, raw captures, merge or device action. Parent retains integration ownership. Full host gate stays FAIL. The original local archive SHA256 is `06d46827c7bfb70cda7f629da8152206c92fa9ecf2701e08942461d07e64873e`; this public text-only packet has its own SHA256SUMS.
