# PR843: missing 88-selector replay evidence

Fresh replay on 2026-11-06: **88 PASS, 0 FAIL, 0 NOT_RUN; exit 0**. Existing 248 logged results are preserved and were not rerun. These are new execution logs and hashes, not recovered original logs.

Checked source: `6c64b1d1604b9b05b4fc7d1f899f7276ace05e70`, tree `577c579037624c3f9092d7c5ba89b6e08bc2a4ca`. All eight declared inputs match the original receipts. Exact Clang 19.1.7 Debian package hashes and TB-3PO reference archive match the original pins. See [input verification](input-verification.json), [execution script](run-first88.sh), [result and individual hashes](replay-result.json), [stdout](stdout.log), and [SHA256SUMS](SHA256SUMS).

[Evidence archive](pr843-first88-replay-evidence.tar.gz) SHA-256: `b24efa891ca5b135300bb78a6e283079bbec830ec4d304d944c3721fec59fa29`. The archive contains all 88 text logs, source/toolchain pins, command, result, and original receipts. Every archived manifest hash was verified.

Strict scoped preflight: 11 PASS, 0 FAIL. Native SDK/package checks were not selected; the runner's generic native-package skip message does not describe a skipped selected test. No native rebuild, device action, merge, firmware publication, or source-owner change. Existing QA and integrator retain acceptance ownership.
