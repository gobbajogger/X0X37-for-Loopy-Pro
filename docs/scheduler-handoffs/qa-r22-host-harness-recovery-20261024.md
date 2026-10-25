# R22 source qualification: six test harness corrections

This is a tests-only repair, not firmware or installer acceptance. No production
source, compiler input, hardware or protected branch changed.

## Exact source and retained evidence

Native Git obtained the complete R22 checkout
`56c180450393f26859419ce13b479e45cb0c242d`, root
`b3cd9031c4ec1693b5a0d1566a51d1c12054d5dd`. It is not a reconstructed subset.
The initial maintained host suite returned **89 PASS / 6 FAIL** (95 selectors,
exit 1). Its failed result has not been relabeled green. Separate existing
install/backup/restore/Node checks passed 22/22, and pinned DSP comparisons
passed 5/5, including 303, 808, 909 and TB3PO.

During verification, sole Safe Merge integrated the independently reviewed
licensing PR #126 as `77f77e43ba84b39024183426709f56b0b368394e`, root
`55cfa45acc82e70cdc79aac99fd52828abfbb857`. Its exact diff from R22 contains only
`LICENSING.md` and the BB Gen MIT notice. The isolated test branch fast-forwarded
to that base without changing the independently reviewed six-file test patch.
All firmware, tools, BLE, assets, host, web and test inputs of the base are equal.

## Why the six tests failed

- `smc-link`, `smc-reconnect-epoch` and `smc-clip-visibility` omitted the existing
  production helper `smc_mmc_drain_stop` when extracting `smc_poll`. They now
  compile that actual helper, rather than replacing it with a dummy.
- `rec-live-smk` models disconnected SMC but omitted the newly required
  project-active/reset fixture symbols. It now supplies the inactive fixture
  contract. The separate **production-linked** `project_smc_runtime` test also
  passed; these fixture stubs do not replace that coverage.
- `smk-apc40` expected the obsolete `sent_note` identifier. The exact existing
  tests-only [PR #69](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/69)
  correction is retained, including the current project-busy/update-entry guard.
  Its new blob is `a07ee479dac12a61bb11947f3c5d51a4c9d74813`.
- `smc-drum-rs` expected help to block REC and clear Solo. Current `BEHAVIOR.md`
  **SMC-02** explicitly says help is a card and controls continue. The test now
  checks REC on/off while help remains visible, R7 closing help, confirmation
  blocking a new REC action, Solo remaining under help, and the other owners
  clearing Solo. Nine stale failures became **124,970 assertions, zero failures**;
  checks were added, not disabled.

None of these baseline failures established destructive installation behavior.
They were not silently waived or treated as passing feature tests.

## Verification after repair

- A separate independent agent reviewed exact test-patch SHA256
  `39b43b3e78c6efed24b10d5c97203345162547f8784051065a2be707e50bae8a` and independently
  reran the affected fixtures. This parent-verified receipt is not human approval.
- Full aggregate of **121 selected applicable tests: 121 PASS, exit 0**, against
  the complete R22 checkout with the repaired test files. The maintained pinned
  reference sources are listed by full commit/tree in the JSON receipt.
- After the docs-only base update: all six affected selectors plus `rec-live`
  and production-linked `project-runtime`: **8/8 PASS, exit 0**. The test patch
  remained byte-for-byte equal. No firmware rebuild was warranted by this diff.
- Both repaired SMC link scripts additionally passed GCC UBSan and ASan+UBSan
  (four commands). LeakSanitizer was disabled; no leak PASS is asserted.
- Real private Elite/Pro packages independently pass product/header/CRC and
  stock-region-map checks. Actual private controller join passes CRC/ABI checks,
  exact software extraction, Python/JavaScript logical-byte equality, and
  removal back to the original package. These are **not device readbacks**.
- Diff whitespace, Python syntax and shell syntax checks pass.

The JSON receipt contains the exact selector command, input identities, baseline
and final counts, immutable artifact hashes and retained-log hashes. No binaries
or raw device/user captures are committed.

## Remaining source acceptance, without adding unrelated gates

The full release gate and new native target builds were not run. The runner's
target-generation package/loader path lacked raw app/loader/SDK inputs and was
skipped; separately checking the real supplied packages does not relabel that
generation path as executed. This is selected full-checkout host verification,
not every repository test or complete pipeline approval.

Existing native Python first-responder/wrong-device OTA hazards are not fixed by
these tests. Do not certify that route from its positive fixtures. Conversely,
unintegrated PR #114's native `physical_id` design is not an automatic dependency
of the separate browser WebMIDI route. Browser PR #8 must independently qualify
its actual loader/model/device binding and backup/restore/controller path.

Existing sole Independent QA and Safe Merge should consume this minimal test
repair and issue an exact-current-source, operation-specific acceptance. Only
the existing integrator may advance PR #103 through the protected firmware
`nightly` PR workflow. Qualified source/model/controller packages, legal notices,
lawful immutable hosting, browser review and sole-publisher HTTP byte verification
remain separate prerequisites to a selectable NIGHTLY candidate. Prior hardware
PASS is not a NIGHTLY prerequisite. Production remains separately hardware-proven
and owner-approved.
