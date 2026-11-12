# Optimizer idle recut qualification

Elite and Pro DEV builds with SDK **V1.2.1** passed. The final full host suite run by this Codex task completed with **336 PASS, 0 FAIL, 0 skip**, exit 0. Controller build and package/controller integrity checks also passed.

The prior optimizer lane included work outside the target profile. This recut uses the exact `wip/one-build` base `48e92b76c2f476cf40771e00a04e1838136bb55d` and preserves the old lane ancestry for a nonforced update.

The base already contains the lane's master and FX compressor idle helpers and their identity tests, byte for byte. The old Chicago restored-insert optimization targets effects outside the base's seven-type policy, so it is excluded. Players retain the supported sound and controls. This stage adds no DSP behavior or new CPU-performance claim.

Tested recut: `2f759f0c1c562f0d312040d4bba37c1800926486`.
Complete built/tested source tree: `00370c8cf385577395f616e9df3cbc45a0d23041`.
The recut and pinned base have identical complete source trees. The final delivery adds only this small text evidence directory; other paths, modes and blob identities remain identical.

| DEV model | Build | Model/CRC checks | Code free | Static RAM free | Pool free |
| --- | --- | --- | ---: | ---: | ---: |
| Elite | PASS | PASS | 72,104 B | 2,524 B | 8,600 B |
| Pro | PASS | PASS | 72,104 B | 2,524 B | 8,600 B |

The full suite includes the optimizer identity/equivalence checks, GCC and Clang optimized/sanitizer cases, package/update checks and every registered scenario. The first full run found one missing-Clang setup failure; after supplying that dependency, a fresh complete run passed. No test or expectation was changed or removed.

`verification.json` contains the public source pins, measured results and hashes of the locally retained full transcript and source-bound run record. Detailed execution logs, workspace/tool paths and input inventories remain local. This summary contains no firmware, controller, SDK/toolchain binary or credential payload.

Status: BUILD_ONLY plus full host proof. No DEV CHECKED record, release, device result or installability is claimed. Next: the assigned integrator reviews the single PR into `wip/one-build`; protected production and hardware gates remain separate.
