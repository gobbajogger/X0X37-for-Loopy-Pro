# Restoration preparation evidence

This is a preparation checkpoint, not restored firmware qualification. UI changes
remain held for owner approval. No source, firmware or controller change has been
implemented on the delivery branch at this checkpoint.

## Exact source

- Base commit: `e2c33498d33c425287f1df077b8ac6cc89a6e8aa`.
- Complete tree: `48246f816a7984871fe39dd92db646fbfeab15de`.
- API reconstruction verified all 4,602 files, blob identities, tree, original
  commit bytes and a clean checkout. Source-recovery scratch accounted for the
  original commit's two trailing message newlines; accepted only the matching SHA.
- Branch: `feat/restore-chicago-fx-bt-idle-extras`.

## Cloud baseline checks

| Check | Result | Scope |
|---|---|---|
| Physical BT/Save held-note/queued-save regression | PASS: 52 checks, 0 failures | existing base, explicit SMK-37 |
| Connected-peer-name regression | FAIL: missing renderer helper and host declaration/guard | expected restoration negative, existing test |
| Elite native DEV build | PASS, exit 0 | unchanged exact base, SDK V1.2.1 |
| Pro native DEV build | PASS, exit 0 | unchanged exact base, SDK V1.2.1 |
| Model/header/app/OTA CRC and slot-map integrity | PASS for both packages | baseline artifacts only |
| Standalone SMK controller build/integrity | PASS, exit 0 | existing source; not inserted into packages |
| Compiler and SDK identity | PASS | compiler pin and 11/11 SDK pins, before/after |
| Restoration-candidate full host/native checks | NOT_RUN | owner UI approval and implementation pending |
| FM-1/MKE builds/tests, release gate, hardware/install/flash | NOT_RUN | outside requested scope |

One initial Elite build failed on the read-only default asset-cache directory.
Retry used the existing supported `X0X_GEN_CACHE=0` setting and passed without
source edits. These successful baseline checks are not claimed for the proposal
or a future restoration candidate.

Both baseline apps: 458,360 bytes; code 458,360/552,672; RAM 95,056/98,304;
pool 334,540/344,064. Controller: 59,740/65,536 bytes; RAM 26,336/26,368.
Final insert storage must preserve existing reserves and be remeasured.

Package SHA-256 (local evidence only; no binary committed/uploaded):

- Elite: `b123c48c98ebed3865e524fb7f597e810bb03b004cb923cad728117cfde06a78`.
- Pro: `47ac36933f1c5620de260838cd5218a6925d798ce387629c6ea2fa4c2e4a4ece`.
- Controller: `710df3a388088b85f69e75ad0340c643d683ca703c1174a279660e238800eb5e`.

## UI proposals

Real 240x240 RGB565 framebuffer PNG pairs are delivered in the requesting chat,
outside Git. They use actual UI/gfx code and native fonts; AFTER frames come from
an isolated proposal renderer. No capture bytes or PAD DIAGNOSTICS code enter the
delivery branch. Injected CPU/peer states are deterministic fixtures, not device
measurements. Proposal FX labels do not establish working audio processing.

## Remaining dependencies

- Owner approval of native BEFORE | AFTER UI proposals, required by the current
  task instruction.
- Resolve exact meaning/source of BT SAVE PENDING. The generic #632 physical
  BT/Save retry already passes; no separate Bluetooth-settings retry was invented.
- Demonstrate insert memory admission, then implement the approved bounded ports.
- Independently review and qualify the final candidate; update the same draft PR.
