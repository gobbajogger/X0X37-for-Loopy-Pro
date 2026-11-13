# SMC SELECT LED native qualification

Final source `2ed626fcb0cb2c1ce1d7ff85ecfbe4e6acb6b1ca`, tree `78aeb867d2dc70fa670d4d63759ff163bd3d3059`, production blob `36d8018012e0c38fd79a0bf0e333d33c1435fb60`.

Both official `tools/dev_validate.py` runs completed as DEV CHECKED and both official verify commands passed on the exact model package/controller bytes. New packages were built from the final pin; historical94061cf/e2b artifacts were not relabeled. `FINAL-QUALIFICATION.json` records complete current identities and digests.

| Model | App bytes | Code free | Static RAM free | Pool free |
|---|---:|---:|---:|---:|
| Elite | 322164 | 230508 | 352 | 21092 |
| Pro | 322148 | 230524 | 352 | 21092 |

The scoped Linux run executed and passed actual registered selectors smc-select-mode, smc-filter, smc-reachability, smc-lock and smc-ui, plus the named select-led-fx-only interaction mutation. The corrected final feedback harness passed1919 assertions in Linux. Its original1880 expectations/5FAIL result remains in feedback-before.log. Source5a7→final2ed changes only that feedback fixture; source and the five tested fixture closures are unchanged. The scope receipts, log hashes, current fixture blobs and byte-identity witness are verified as declared immutable inputs in each official model validation plan.

`function-closure-map.json` proves production bytes outside smc_led are identical to the prior qualified input. The changed function only reads SELECT mode/overlay/clip state and the corresponding source FX bit. Prior unrelated transport/input/mutation proof is retained under that scope mapping. The old eight interaction mutations were not needlessly repeated; the new LED runtime mutation was executed. No unrun whole29-suite claim is made for this source.

Compiler/SDK/image reuse was freshly verified by content identity. Actual worker is immutable Linux/amd64 image0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1; actual compiler check reports Clang4.0.1. Complete SDK/toolchain/runtime closures match before/after each official run. The official validator rebuilt the controller as its required component and actual resulting bytes matched the unchanged prior controller closure. SHA256 is956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a; whole-file CRC329c07ae67; maintained header CRC3277b7a496. Maintained header/ABI/bounds/CRC check passed. These CRCs are not physical readback claims.

The official validator removes scratch raw-app files after completion. `extract_app_proof.py` uses the existing package integrity, deinterleave and packer SFC functions to recover the compiled app bytes from each exact validated package. It checks the full padded app-entry CRC16, verifies erased padding, and uses the verified build-log app length. App/loader hashes and whole-file CRCs are recorded separately without modifying official records.

Labels remain Unversioned legacy DEV / DEVICE NOT TESTED; no numeric allocation or physical LED/device claim. Separate bass/legacy compatibility integration is not covered. All owned workers and temporary worktrees were cleaned. This executor made no source edits, commits, GUI/device commands, flash, merge or publication. Do not commit/upload local .fwsc, ctl.img, recovered app binaries or proprietary inputs; sanitized JSON/logs/hashes are evidence only. Parent independent final review and authorized integration remain separate.
