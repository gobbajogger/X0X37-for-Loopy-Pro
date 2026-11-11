# LX-REQ-016: bounded PR861 source-input QA closure

The original PR861 source delivery and its receipts remain intact at
`eb2a81deb8247d2387b4e8edd0691163877d523c`, tree
`aeeb1505141a36d9baba22894fb6dffa639c7488`. This follow-up preserves the
completed source-stage grant under the owner's separately extended qualified
NIGHTLY objective. It does not claim completion of that downstream objective.

The retained 29664-byte [optimization stdout](optimization-equivalence-original.log)
has SHA-256 `513cab4751610821e42cf4e6da9611338eac06fa9ff6a64fe8fd03c3371fb052`.
[closure.json](closure.json) maps commands and PASS output for six nested groups:
clip latency, master A3, drum808 A3, fxbus A3, master idle and fxbus idle.
They ran under one producer-owned selector in the original full suite; they are
not six independent QA executions. No unchanged full suite or native build was
repeated. Original tool executable hashes, all 11 SDK input pins, all seven local
artifact hashes and the original host-suite log digest still match their receipts.

The missing donor Chicago insert-idle fixture tests types 3/12/13/15/16 and 26
factory routings outside the selected compact policy. That policy admits only
0/4/5/10/18/22/24. Its required `chicago_fx2_inserts.h` is absent from both selected
heads. The existing recut manifest already excludes the restored-insert-dependent
optimizations. The donor's identical `chicago_program.c` does not provide its
missing insert architecture. This concrete source-policy exclusion was delivered
to the sole integrator in [PR698 comment 6675682814](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/698#issuecomment-6675682814).
It is for the integrator's disposition, not a unilateral waiver or a 7/7 claim.
If the selected architecture changes to restored inserts, its donor fixture pair,
registration and affected positive/negative execution become a real dependency.

The two USB-v2 fixture files exactly match donor
`d010d7dd5e3ddb027d9589ab3cef71ab4b007386`. Production USB remains blob
`b22a79215072ee178351e42f35ed2678dde9f360`. The original top-level runner and
`run_usb_retry.sh` are unchanged; `usb-retry` remains registered.

Producer execution with GCC 14.2.0:

- [V2 baseline](usb-v2-baseline.log): exit 0.
- [Old fixture, missing-reset mutant](old-fixture-mutant.log): exit 0; the mutant
  survives, demonstrating the earlier test gap.
- [V2 fixture, same missing-reset mutant](v2-fixture-mutant.log): exit 1 as
  required, with the dead-SIE assertion reporting `up 0`, `timeouts 3`, and
  `ticks 600000`.
- [Unmodified source after isolated mutants](usb-v2-after-mutant.log): exit 0.

The mutants remove only the timeout reset in `usb_retry()` from copied source;
production files are never modified. Run the reproducible check on this source:

```
CC=cc sh tests/host/run_usb_retry.sh
CC=cc python3 docs/lanes/build25-usb-recut/qa-closure-20261106/reproduce_usb_timeout.py --root . --out /tmp/pr861-usb-timeout-proof
```

The original 337/0/0/0 receipt belongs to its original pre-v2 fixture input.
This follow-up adds scoped producer proof and reuses unchanged evidence; it does
not relabel that historical suite as a new full-suite run or independent QA.
Original source-stage completion does not waive the pending strict ROADMAP
57-row inventory, pinned invocation or cross-root runner/checker findings.
Those remain with the existing strict-gate producer, sole integrator and same QA.

Return this exact changed closure to existing
`independent_qa / 6adafbbf54088191a86715d49e7dc6b0`, retaining its prior finding
IDs and review budget. The sole PR698 integrator/canonical writer must adopt the
new owner acceptance in the existing LX-REQ-016 flow, disposition the Chicago
exclusion and choose the complete immutable combined input root for existing
Build and QA. Native/package, safety/data/backup/negative/model/hash/CRC/memory,
licensing, hosting, integration, catalog and publication acceptance remain with
their existing owners. Receiver execution is not inferred from comment readback.

No merge, publication, device operation, successor creation, `.github` change,
binary/token push or canonical queue write is part of this source-producer work.
