# SMC Transport — #89 → #94 → #50 closed stack (2026-10-24)

Source-owner reconciliation, NOT independent QA or hardware qualification. Consumes #50 at `ed03519ccb59f945814efd51bf3ec7486cdd7439` and stacked #94 at `9083dc35a5e270930323b44dde7bdc5916f826f1` (parent #89).

- `firmware/src/ble/ble_midi_host.c`: retains #50 mailbox/credit/ATT/reconnect handling while adding the exact #89 SysEx length preflight and #94 ACL PB0/PB3 first/complete packet rules.
- `tests/run_tests.sh`: explicitly registers both inherited linked-source tests, with existing #50 registrations preserved. All #89/#94 regression tests and evidence are inherited as exact Git blob hashes.
- Restores `docs/rules/firmware-nightly-promotion.md` in this former pre-nightly source branch, so its combined Git root contains live NIGHTLY/production guards.

## Executed this run (local source-lifted regression only)

- `bash replay.sh` from previously attached `LoopyX0X37-SMC-Link-Epoch-20261024-Evidence.zip`: exit 0, 6/6 before runs exit 1 (5/20 assertion failures), 6/6 after runs exit 0 (20/20 PASS); GCC and Clang, plain/UBSan/ASan+UBSan, compilation exits 0.
- `bash replay.sh` from `LoopyX0X37-SMC-Zero-ACL-Credit-20261024-Evidence.zip`: exit 0, 6/6 before runs exit 1 (4/18 failures), 6/6 after runs exit 0 (18/18 PASS); GCC and Clang, plain/UBSan/ASan+UBSan, compilation exits 0.
- Exact Git source overlap, patch uniqueness and test-registration checks: PASS. #89/#94 original producer tests are inherited historical evidence, not newly repeated full-tree QA.

## Remaining gates / next owners

**Independent QA:** compile and exercise the actual full combined C source and negatives. **Hourly Build:** exact SHA Elite/Pro/ctl BUILD_ONLY can proceed immediately without QA/merge. **Sole Safe Merge:** after QA, integrate source with first NIGHTLY candidate and Data Safety. **Catalog/Publisher:** independent full prehardware and immutable download/HTTP checks before offering to owner.

**Disposition:** SOURCE RECONCILED, REVIEW READY, full linked C/target builds/independent QA pending; DEVICE NOT TESTED, NOT INSTALLABLE.
