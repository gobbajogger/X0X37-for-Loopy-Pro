# Cursor-led firmware version continuation

Checkpoint 2026-10-25 14:39:48 UTC. Owner requested urgent usage-limit handoff. Local Codex has stopped at a safe boundary. **No native build, source write, allocation or device operation is in flight. Do not replace or duplicate the Cursor writer.**

## Owner and exact source

- Existing sole Cursor writer: `bc-a9299d6a-4560-4a73-9ac6-d093c0ffcdef`, https://cursor.com/agents/bc-a9299d6a-4560-4a73-9ac6-d093c0ffcdef . Cursor public cloud VM; observed grok-4.7-high-fast, STANDARD/MEDIUM unavailable. Writer acceptance: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/53#issuecomment-6398652115 . Follow-up mentions have consistently routed to that same agent.
- Existing open draft PR53: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/53 . Branch `lane/firmware-build-version-labels-20261023-1814`; target nightly. No protected merge.
- Latest remote-observed and local checked-out head `f6c4c23f6822535cae25625cf083ae4ebed10ae5`, tree `2a34a88f1c8e21e706cb902d9fa1e6eb9e70bcc6`. Clean detached local checkout `repo/`.
- Original input `253b5101a9e3ebcc1e8dcca48764322909820b26`; first delivered implementation `8d7671ecb23759c316d1d5de04532ca171a06625`, tree `e62e630bbe0545e615693c53c1fe966b1176102d`. Nightly `08d3164a265123cb4c84e5104642fd93257d63d7` reconciled into isolated source branch only.
- Latest code is **NEEDS CHANGES**, not accepted. Cursor queued repair mentions: comments6398880350,6398890356,6398910770,6398975579 on PR53. Final writer progress/results reader remains PR53 comments/head. No assertion of source completion.

## Implemented and independently checked

Compact `X.Y.Z_openbeta` beside ABOUT X0X; numeric `X.Y.Z` macro/product remains separate. Real headless selected-row render independently executed/viewed: x127 width77, name/arrow fit. `about-row-preview.png` is fixture `0.1.3_openbeta`, NOT issued version; SHA256 `d0c52969961d5f83039c729dd07fbbd9d26d4a2525161cc60649041e7c7ac8d7` (local Pillow10.4 vs Cursor13).

At first candidate: local allocator5/5, release-publish3/3, inspector6/6 PASS; these tests missed blocking real-world paths. Independent reviewer reported NEEDS CHANGES. At f6c: independent allocator6/6 PASS, actual Git40-character identity/existence, changed-source allocation and isolated source-preservation paths improved. Unchanged preview evidence reusable only if firmware/font/build inputs remain unchanged. See `review.md` for exact findings/reproductions; preserve historical review.

## Still blocking before real allocation/native build

1. **Documented `--remote origin` fails**: isolated record checkout receives literal URL `origin`; remote probing also relies on ambient cwd instead of source `--repo`. Reproduced real Git source: alias fails; absolute bare URL succeeds; alias after branch exists fails fetch. Resolve named source remote first; add CLI tests both source cwd and `--repo` from elsewhere. Last repair comment: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/53#issuecomment-6398975579 .
2. **Numeric acceptance bypasses allocation/immutable receipts**: build/dev_validate/package/publish did not consult ledger at latest inspected f6c. Existing test accepts0.1.3 with empty ledger; `_kept_versions` silently removes prior same-version entry before replacement. Require allocated exact source/version and exact model/package/controller receipts at acceptance; reject changed same-version catalog/source/hash, preserve build-before-bind/no circular gate and truthful legacy. https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/53#issuecomment-6398880350 .
3. Null/non-string catalog product raises AttributeError instead of structured refusal; repair/test. Fabricated source IDs were accepted at8d, fixed for serialized issuance atf6c; verify final paths.

No real version issued. Source `releases/openbeta/ledger.json` allocations empty; coordinator never pushed `release-records`. `0.1.0` is proposed initial ledger number, not allocated. Existing Git tags/GitHub releases query returned empty. Never derive version from commit count/hash or fixture42. Verify actual final allocation branch state before issuance/retry.

## Native local obligation (after final independent acceptance)

No builds run; do not claim DEV CHECKED, matching native artifacts or installed identity. Detailed verified preflight: `native-preflight.md`. Prepared parameterized bridge `native/run-version-build.sh`, `native/capture-inputs.py` passes syntax only; adapt to final ledger-enforced CLI before execution, it currently uses direct `build.sh --release <numeric>`.

- Cached linux/amd64 image `sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1`.
- Toolchain `[owner-local path withheld]`, content SHA256 `50a35938b8c15394e2a5d6bbd9284dcb3a9ccfc14796a15f820ff5f8b5308ba5`.
- SDK `[owner-local path withheld]`, content SHA256 `f55b7fe7fc6f5fbeaf13eba0299d6758deae1bd8741e45d80e8449c0a280f9e3`. Default SDK differs; do not substitute it.
- Stored qualified controller copied locally to `native/reused-ctl.img`:65492B SHA256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`. Originals remain untouched in `control-native-qualification/native/{elite,pro}-qualified/ctl.img`.
- Controller closure matches inspected nightly: ble tree `6336915688f7b9178d17f8975703c108aa464ec0`, ABI blob `8716949b32a1bb11fb3bf5f338f0265dffae9de9`, locked checker `f1ac9fe3f1535c01cfd37016e158472740a1bc8d`, settings `01b36850050ae2de9b7f973b16bac5823aaef4cb`. Recompare final candidate; reuse unchanged controller, build only affected Elite/Pro apps. Prior qualified Elite had244 code bytes free; measure real new fit.
- Docker socket requires normal sandbox escalation; read-only preflight escalation approved. No auto-review rejection. Container builds use `--network none`, source mount read-only, private SDK/toolchain mounts read-only and isolated task output. Capture before/after complete inputs, app/package/model/loader/controller identities, CRC/budgets and actual exits. No source/toolchain uploads.

Native step remains with qualified local handoff coordinator, not cloud Cursor. Allocate real number atomically only after final source/review accepted; execute both models from exact pin, inspect actual product/hash, bind separate exact receipts and verify rebuild/collision negatives. Source changes affecting compiled inputs invalidate affected outputs; preserve unchanged evidence. No publication or flashing.

## Other scope and authority

Catalog owner remains website PR13: https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/13 . Notice https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/13#issuecomment-6398583823 . Source metadata contract `repo/docs/release-metadata-contract.md`; no catalog owner acceptance yet. Approved layout publisher completed/released separately; do not rewrite its layout or catalogs. Preset/clock/engine scopes independent.

Installed qualified unversioned source `892bf60d912eca93597dcf03be25a9756540c949` and all its immutable artifacts stay untouched; no further flash. No installed version inferred from renamed files. Source/test/docs and draft isolated PR updates authorized; no protected main/nightly merge, Actions, GUI, credentials, public binaries/releases/site publication or device operations. Final release/publication requires owner approval.

No active local exec sessions, no compilers or pending tool cells. Independent review helper finished; no local source writer. Root direct message tool could not resolve parent thread (thread not found); no retry/bypass. This packet and PR reader are the continuation route.

Confirmed transfer: native send_message_to_thread succeeded to sole cross-project coordinator `01a17381-d6ad-7488-ae04-7dd2ef040038`. Packet paths, writer/pins, pending findings and safe-stop/local obligations delivered. Recipient acceptance not yet read; no new writer or successor created.
