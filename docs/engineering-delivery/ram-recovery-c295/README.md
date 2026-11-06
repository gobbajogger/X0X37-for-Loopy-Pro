# RAM proposal: host verified, native NOT_RUN

This isolated proposal reuses the current output window for the 909 mono kit send,
removing its dedicated 256-float scratch without changing arithmetic or render order.
It preserves existing features, audio buffers used by CPU2, UI/SMC code, display cadence,
pool reserve and safety margins. The end-user intent is to retain all combined features
while recovering static RAM; no target-memory saving is claimed until native measurement.

Base commit: c295af1554f9e80a29864cd013995eccad66cd2c.
Base tree: d98072ba1d689cb99bd16e4a6e1e682b22b04c70.
Tested source/test tree: 4a0512ce01628119460c1bffad1b85f80027f172.
Patch SHA-256: 1942c173c8fa481b15b5173555f414f525f2c8ef4b6bd97982730403e97660a3.

The supplied archive was verified against SHA-256
21263b56d2058e9f0e434a13dec69e3485dcb77af0d896c140643b7b1841d6ab.
Its actual Elite and Pro ELFs each use 98276 B static RAM (28 B free) and
335624 B pool (8440 B free; 8192 B minimum retained). Existing native mono
storage is 1024 B. These are baseline measurements, not after-build evidence.
LCD state uses 1536 B DMA storage, 540/360/120 B tile hashes and 12 B flags/hashes:
2568 B total, replacing 672 B row/column hashes, a 1896 B net object-size increase.
None of that live LCD state was removed or shared.

## Executed proof

- Host normal and ASan/UBSan comparisons: PASS, 1600 render blocks per executable,
  exact PCM and meter streams, 4670 CPU2 jobs/3736 joins.
- Host-only assertions: every sub-block preserves preceding output windows and
  the not-yet-rendered suffix. Eager/deferred completion, failed/partial CPU2,
  split blocks, explicit nonzero 909 sends and asymmetric pan are covered.
- Intentionally unsafe mono=dry_l alias: detected by different output.
- Existing LCD bounded-tile tests: PASS; exact pixels and bounded DMA.
- All 12 original-engine private input pins verified; bytes are excluded here.
- Initial uninstrumented host object BSS: 246744 -> 245720 B (-1024 B).
  This is a host measurement, not a target linker result.

Reproduce with admitted original-engine inputs:

```sh
sh host/build_host.sh --generate-only
python3 tests/host/engine_mono_scratch_test.py
sh tests/host/lcd_bounded_test.sh
```

Independent review: the parent delegation reports no defect in its independent
host-scope review. That report is not a native, hardware, performance or release
approval; this evidence branch does not invent a reviewer receipt or wider scope.

## Native blocker and handoff

New Elite/Pro native builds: NOT_RUN. The supported Library compiler/SDK download
helper returned HTTP 401 at authenticated tools/list before transfer. No account,
ACL or OAuth scope was named; no missing permission is guessed. No pinned copy
was found in the same workspace. The route was stopped, not retried or bypassed.
Do not switch executor/identity/download route to evade it. See NATIVE-BUILD-PLAN.md
for legitimate authentication repair and exact same-environment before/after proof.

This branch is durable source/evidence only. No shared integration branch changed;
no merge, firmware publication, device action or target-memory acceptance follows.
The existing integrator retains source selection and must resolve native evidence
before accepting this proposal into the combined candidate.
