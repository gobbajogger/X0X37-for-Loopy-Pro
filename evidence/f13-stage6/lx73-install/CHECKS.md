# F13 Stage 6 — LX-73 install (BF-012 after #308)

| Field | Value |
| --- | --- |
| Agent | `bc-4b6297d2-7ec1-5721-930d-18248d7bce99` |
| Evidence tip | `ffe99f47527245a674ebb69353bf034b4cac7620` (#319 companion logs) |
| Install commit | `092589b84fd455355216cb0de93c4782236f7524` |
| Content tree | `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af` |
| Config | `X0X_DEV_PADCAL=1` |
| Device | **present** — MIDI `X0X FM-1` (Elite) |
| Elite result | **INSTALL BLOCKED** (attested package bytes missing) |
| Pro result | **INSTALL BLOCKED** (attested package bytes missing; hardware is Elite anyway) |
| Flash | **not attempted** (no DEV OK on attested hashes) |
| Publish / nightly / main | not touched |

## Attested hashes (Stage 5 LX-73)

| Model | Package SHA-256 | Controller SHA-256 |
| --- | --- | --- |
| Elite | `9c1168cf83d298e02863441df63f09a0508c00e8a908f04dbae79d532dfdd1e5` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Pro | `3a29723eb62024eccd7b010d97912e21eabad4ec1a933b90b428ad8f63e87a07` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |

Controller `ctl.img` is on this Mac. Package `.fwsc` bytes for those SHA-256 values are **not**.

## Gate (this host)

```
tools/require_gate_pass.sh 092589b84fd455355216cb0de93c4782236f7524 \
  --config dev --fwsc <pkg> --ctl <ctl> \
  --dev-record evidence/f13-stage5/lx73-devcheck/{elite,pro}/record.json
```

Against local rebuilds of the same commit/tree:

- Elite rebuild sha256 `0e62888b7fc83ddaa077f3dd573c068c4286957e3076b3996a119e13199ae5fb` → `REFUSED: package differs from validated development artifact`
- Pro rebuild sha256 `b4e95e1ed7b992c177225e67beb051b0892498471ec7511a3a1890522cdc1056` → same REFUSED

Stage-5 records were produced on Linux `6.12.94+`; this Mac worker image is Linux `6.8.0-117-generic`. Same content tree and budgets; package digests differ. Did **not** bypass; did **not** flash rebuild bytes.

Companion logs on tip `ffe99f47`: `elite|pro/{build,check-0,controller}.log` + `record.json` present.

## Unblock

Place attested bytes into the LaunchAgent artifact cache (or any path `find_local_hash` can see):

- `~/Library/Application Support/x0x37/unattended-install/artifacts/9c1168cf83d298e0/x0x-smk37-elite.fwsc`
- `~/Library/Application Support/x0x37/unattended-install/artifacts/3a29723eb62024ec/x0x-smk37.fwsc`
- ctl already at `…/artifacts/956e3e214658aeaa/ctl.img`

Inbox already holds LX-73 `LOCAL-INSTALL.json` (`role: stage6-install`). Re-run gate → expect `DEV OK` → `fm1_install.py --commit 092589b8…` for Elite on `X0X FM-1`.

## Stage 7

No install PASS → no `SUCCESSOR-STAGE7.txt` from Stage 6. `CURSOR_API_KEY` unset on this worker → do **not** claim Task as Stage 7; coordinator CreateAgent still needed for Stage 7 publish of these hashes.
