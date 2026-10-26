# F13 Stage 6 — LX-73 install PASS (Elite)

| Field | Value |
| --- | --- |
| Agent | `bc-4b6297d2-7ec1-5721-930d-18248d7bce99` |
| Install commit | `092589b84fd455355216cb0de93c4782236f7524` |
| Evidence / stage5 tip | `32dc4f61` (#319 prerelease URLs) |
| Config | `X0X_DEV_PADCAL=1` |
| Elite | **PASS** record `4cd9e2e9a47b63b1` |
| Pro | **DEV OK** gated; not flashed (device Elite) |
| Package source | GitHub prerelease `lx73-092589b8-dev` (not Mac-local rebuild) |
| Publish / nightly / main | not touched from Stage 6 |

## Gate

```
require_gate_pass.sh 092589b8… --config dev --fwsc <attested> --ctl ctl.img \
  --dev-record evidence/f13-stage5/lx73-devcheck/{elite,pro}/record.json
→ DEV OK (both)
```

## Install

`fm1_install.py` Elite attested package → exit 0; post-info `SMK-37 Elite_900`; Bluetooth block CRC32 `4a400899` match.

Local record: `~/Library/Application Support/x0x37/unattended-install/records/4cd9e2e9a47b63b1/`

## Stage 7

`SUCCESSOR-STAGE7.txt` pushed for coordinator CreateAgent. No `CURSOR_API_KEY` → do not claim Task as Stage 7.
