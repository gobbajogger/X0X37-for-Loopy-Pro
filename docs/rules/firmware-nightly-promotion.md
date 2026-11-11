# Firmware NIGHTLY integration and production promotion

The firmware source repository has two persistent branch roles:

- `nightly`: reviewed integration of development firmware source. Specialist changes are proposed from isolated branches via PRs targeting `nightly`; sole Safe Merge owner integrates compatible work only after actual appropriate native tests and independent QA. A draft or failed PR remains unmerged and is not a checked build.
- `main`: hardware-proven production source. No automated nightly-to-main merge. Promotion requires attributable owner-operated physical Elite/Pro testing on the corresponding exact firmware AND controller hashes, independent QA, validated nightly checkpoint, and a separate explicit owner production approval. Device evidence from one model never certifies another.

The separate `gobbajogger/x0x37-for-Loopy-Pro_nightly` repository remains GPL browser/status/Pages only. Do not mirror firmware source, vendor SDK, DSP references, toolchains, firmware `.fwsc`, `ctl.img`, secrets or user samples into it. It may reference exact checked source SHA and legally distributable immutable externally hosted test artifacts after byte/hash validation.

NIGHTLY is intended for the FIRST owner hardware test. A `DEV / HARDWARE TEST — DEVICE NOT TESTED` candidate requires actual pinned as-applicable Elite/Pro DEV target builds and matching controller, model/header/CRC/SHA256, memory/flash budgets, host regressions, backup/readback/restore and installer failure negatives, independent critical QA, licensing and verified immutable hosted bytes. It does NOT require prior physical PASS, production main merge or separate per-build owner approval. If any software/package/distribution check fails or was not run, record history only and leave the installable picker empty for that candidate.

Do not force-push, directly write protected production main, bypass a GitHub safety refusal, use GitHub-hosted Actions for firmware tests/builds, flash devices, or claim PASS without real exact-source test/build evidence. Preserve legacy PRs and stacked dependencies until individually reconciled. Record full remote source SHA, test commands/exits, artifact hashes, QA, status, and next consumer for each handoff.
