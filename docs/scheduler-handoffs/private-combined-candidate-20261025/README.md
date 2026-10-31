# Private combined candidate: SMC screen, editor dispatch and bundled controller

Build: DEV, BOARD=smk37, X0X_DEV_PADCAL=1. Replaces: no installed or published build.
Checked against decisions: private owner-local candidate only; no protected merge,
publication, Pages deployment, GUI or hardware action. No user data was accessed.

The history-preserving merge combines exactly PR143 `784607fcb21ec9a90bcbae7c6b9a0118f4c71dfe`,
PR141 `33434f47228e54a8ed949f6ea513902727fa2d18` and PR142 `04cb592c47f4b8d5049f5399d7f6f45b50b92c2c`.
Source/build commit: `0e3e4d572e2466872e1ddd1986ff34b9e77dddd3`.
Complete root: `441970c541c8c7abad0030d6f70c2c2fb484a080`.
Compiled firmware tree: `5326e3c214cbe8d133b748685a2259d759e72c97`.
Base nightly: `54f2ffbc3124f7449a69067dc8215a2d91a2e675`.
The following documentation commit adds evidence only; it does not change compiler inputs.

The temporary full SMC control screen returns to the previous application UI.
The editor/OTA main loop classifies each shared pending frame once, preserving
late editor requests. Every new private model package contains its exact controller.
Historical hardware timeout causation and resolution remain unproven.

## Verified owner-local artifacts

| Model | Bytes | SHA256 | Whole-file CRC32 |
| --- | ---: | --- | --- |
| Elite | 651396 | `812c1565e2bcf9182a6c2a97115cfbb0b95514f523bc35bc38a8f0a45bda2b66` | `85318616` |
| Pro | 651392 | `f7d947fce392a2ee179e277940ce43e343e79d5d099954f543d9f0c95019ae97` | `d993904c` |

Both model builds and a cache-disabled matching controller build exited 0 in the
same pinned task14 offline native environment. App code is 549804 B with 2868 B
free; static RAM is 97892/98304 B (412 B free); pool is 335740/344064 B (8324 B free,
minimum 8192 B). Product identities are exactly SMK-37 Elite_900 and SMK-37 Pro_900;
target is AC791N / pi32v2 r3. Package layout, nested CRCs, chip identity, model,
slot-map, exact embedded controller, erased padding, SHA256 and bounds passed.

The private controller is 65492 B, SHA256
`956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.
It has ABI 2, 44 B flash free and 544 B RAM free. Its retained ELF has SHA256
`a53173f0b45a88b6395160ff4317c2b5dc4c92645f9ef79511aa24b8da8a9b16`;
map SHA256 `0e681ad89a50b243644488742b8105660e0a6510fbe978dc463e8e16e9e4b36f`.
Regenerating the image from that ELF reproduced ctl.img byte-for-byte. These private
files remain on the owner machine; only their hashes and receipts are in Git.
A separate component-notice reviewer can inspect that retained ELF/map. Public
component notices are outside this task and did not block the private build.

## Independent affected checks and reuse

[Source review](source-review.json): ACCEPT, no source findings. Twenty selected
combined suites passed, including 191 UI checks and four detected runtime-negative
mutants; editor dispatch ASan+UBSan passed 34/34; UI ASan passed 191/191. Existing
PR143 source/render and 16 affected-suite evidence, PR141 baseline 22/34 to 34/34
review, and unchanged accepted base evidence are reused only for matching inputs.
The known baseline fastmath.h:57 negative-shift UBSan is unchanged and was already
independently reproduced in task14/task16. No clean full-engine UBSan claim or
unrelated DSP correction is made. No release configuration or full gate was run.

[Package review](package-review.json) records isolated installer PR16
`cc7e7e8e68717acda8c774d4da8ca7d5cc401ff8`, root
`7600043f26378ff575e22bc8a8ee245e4a044d35`, stacked on PR14. Only isolated source
was used for package/parser/admission checks; its branch and live catalog remain
untouched. Exact previous browser evidence is reused where shipping blobs match;
no GUI ran. Parser and simulated admission checks are not hosted-HTTP or physical
installation evidence.

## Next separately authorized native hardware preparation

Use the proven isolated task8/task9 native-prep updater contract: validated plain
model package plus explicit exact controller must deterministically reproduce the
bundled bytes above. Do not use this repository's older native CLI as a substitute
for the proven stricter singleton/model/reconnect/readback implementation.

The old private native-prep adapter is intentionally exact-candidate. In
`tools/accepted_build_mapping.py`, ACCEPTED/TREE/BUILT/BUILD_TREE, receipt schema/hash,
actual build log hashes/budgets, independent review and updater pins bind the old
candidate. `flash-handoff.py` pins old source/artifacts. `known_app_only_head()`
pins the old plain logical image, model and supplied loader. The old adapter is
expected to refuse these new bytes; that is not absent parser or package compatibility.

A bounded next native-prep task must map this authentic receipt and exact new
source/artifacts/reviews through that existing adapter format, preserve BUILD_ONLY
and scoped-review status, and independently test positive and changed-pin negatives.
Bind the exact new plain logical SHA and model in the head guard; preserve supplied
loader header/CRC/attributes/entry/length/bounds, reference chip-key/app-header CRC,
sole compatible MIDI pair, strict model checks, staged-loader validation, fresh
atomic slot backup, interruption handling, controller full-block readback and
sample restore verification. Elite and Pro need their own model/reference binding.
No generic full-gate requirement or firmware compiler rerun follows from this
record-format adaptation. Never synthesize DEV CHECKED or use force/guard bypass.

Hardware remains NOT TESTED and is not authorized by this receipt. After separate
hardware authorization, the native owner must do the fresh physical model/port
preflight, use a new backup path, and test SMC full-screen/return behavior, editor
requests and project transfers, Bluetooth controls and data preservation. A host
race test alone does not prove the historical timeout fixed.

The task17 owner-local HANDOFF.md gives exact retrieval paths and the bounded next
task. Existing Safe Merge, hourly Build and publication owners retain their roles.
Source is delivered on an isolated draft branch only. Production and nightly refs,
installer catalog, public downloads and user data remain untouched.

## Separately completed controller notice assessment

Task18's independently reviewed owner-local assessment applies to the exact same
controller SHA256 `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a`.
All ten crypto/CCM implementations were eliminated from these image bytes; their
conditional source questions do not block this private candidate. Five libc and
seven compiler-runtime members remain. Public-distribution completion is limited
to the vendor optimized-libc notices/runtime revision or modifications manifest
and actual recipient-accessible MPL covered source, alongside existing app duties.
No vendor prohibition or generic runtime-source publication condition is asserted.

The assessment is `/Users/mac/Documents/Codex/2026-10-25/task-18/controller-notice-assessment.md`,
SHA256 `cc23341a597887875db27148c7c5137567409dd19022f2ad138b69dbe31e6946`;
its receipt is `task-18/delivery-receipt.json`. Full notices remain a local draft at
`task-18/THIRD_PARTY_NOTICES.draft.txt`; nothing was published.
Exact covered controller source: <https://github.com/gobbajogger/X0X37-for-Loopy-Pro/tree/0e00c262348a1574c37f27e37a60531faef44cb4/ble/ctl>.
Shared ABI source: <https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/0e00c262348a1574c37f27e37a60531faef44cb4/firmware/hal/x0x_ble_abi.h>.
Those sources equal the candidate controller inputs. Recipient HTTP availability
was not verified by task18. This does not block private source/build qualification.
