# Build Catalog Sync — truthful model failure and source history, 2026-10-24

**Role:** Sole NIGHTLY website build/status history writer. **Status:** isolated branch only, independent QA pending.

Repository: `gobbajogger/x0x37-for-Loopy-Pro_nightly`; owned branch: `lane/catalog-nightly-preview-history-reconcile-20261023-1541`; checked parent `dd4555657aa0c40f9ae220df7ca4b7e545fb53d2`.
Firmware integration ref remains `nightly@58b5b45a2dd90581de7a674c331ca5cbe8a67c7e`; nightly site main `1a176e156b049f476074b77d7e6d54be03e8c9ad` and gh-pages `9c5d3b6b0846f89436492851fef89ed3829b7ad7` were read-only.

## Reconciled six new producer checkpoints
- **#93 / `74389d4bb4abc2f8a36b91bc16d1822b63264986` / tree `aeacf2c81bd19a33dd50ba0bf3a6e72b1131271a`: FAILED** — BREAK reverse checkpoint native Elite/Pro pool budget failure. Fix pool memory reserve, recompile and verify both models; no build PASS, independent QA or distribution.
- **#94 / `9083dc35a5e270930323b44dde7bdc5916f826f1` / tree `447582f9bf505c2d448010b7ab23dd36e1ed7a33`: SOURCE-ONLY** — SMC BLE ACL PB0/PB3 valid first-packet handling. Integrate with PR #89 and conflicting PR #50 through sole Safe Merge; native model+QA and distribution gates pending.
- **#95 / `17631165c251aca1c6183e0b0fe85876887bea49` / tree `5bcfa5f8b54c4982b0d41351a724c1ad06c9d048`: SOURCE-ONLY** — 303 ping-pong live pattern shrink boundary repair. Independent full-source QA, sequencer conflict reconciliation and pinned target model builds remain open.
- **#96 / `3473d86ec88c079a6c69f133774104ba4ec8c4de` / tree `c42534d8540ec278d3c3f2c1753f37021abef2d8`: SOURCE-ONLY** — Reject malformed SMP_INFO slot maps before OTA backup. Independent sample-backup/readback/restore negatives and ordered integration, firmware build and installer gates remain open.
- **#97 / `8344f586e57512f651965f3ac21545b24a6c6241` / tree `f73dd236fd1788a87c4e5d1a55a4f5e5158a2303`: EVIDENCE-ONLY** — Installer EN/JA screen-reader label localization. Independent stacked browser/negative QA, single firmware-source integrator and designated Pages publisher pending.
- **#98 / `9506a68aa2d1f0e04eca23871368d18ab2a070cd` / tree `b8b429bfffa251994b64a3066f90bcf5aea833a7`: SOURCE-ONLY** — TB-3PO mutation audible root-note preservation. Independent RNG/model correctness QA and native target model builds pending on this source-only repair.

## Genuine build failure: PR #93
GitHub PR #93 includes independent native Linux JieLi Elite and Pro builds on exact scoped firmware subtree `3fe98ec515b3abafd50a62280632839c2910dd80`: both build commands exited **1**, with pool use 339580 / 344064 bytes, **4484 bytes free <8192 required**; neither produced a valid model package. Controller-only build exited **0** privately. Source and Git-root tree were independently read. The catalog did not independently rerun the compiler and could not materialize the private native-log ZIP (`This Project file does not have an authorized raw-byte materialization path.`). Historical successful model packages remain separately built/unreviewed and are not assigned to #93.

## Catalog and fail-closed schema
Full read-only status grows **2 legacy DEV generations + 69 → 75 other checkpoints**, no downloads. Tighten build status allowlist and reject blank build/history IDs or IDs repeated across builds+history. Add six targeted native Node negative fixtures. Preserve all old entries and generated docs/source JSON exact-byte equality; installable preview stays empty. Website `AGENTS.md` is not among PR #3 changed paths, so canonical Nightly main policy is not altered.

## Checks and limitations
Connected GitHub read back exact PR refs, root trees and documented producer command exits. In-memory V8 full-manifest shape/preservation checks **PASS** with 12/12 malformed mutations rejected. Native standalone focused metadata checker **PASS**, exit 0; its content is not the exact combined website checkout. Current full native `npm run build`, `npm test`, whole-site Chromium, independent QA, firmware target builds by this role, and Pages HTTP byte checks are **NOT_RUN**. Native shell GitHub lookup failed DNS/exit 128; connected GitHub reads and authorized isolated writes are separate. These limits do not block truthful noninstallable history.

**Next:** Independent QA validate exact stacked site PR #3 on combined main + owned head, native npm/browser negative tests; then sole Nightly Main Merger considers reviewed non-draft PR. Sole Nightly Pages publisher handles any lawful candidate HTTP distribution after all prehardware safety gates. Production requires matching per-model owner device proof and separate approval.

**ELITE:** NO INSTALLABLE YET. **PRO:** NO INSTALLABLE YET. No new Pages publication or device flashing.
