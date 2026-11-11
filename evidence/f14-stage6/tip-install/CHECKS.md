# F14 Stage 6 — tip packages install PASS (Elite)

| Field | Value |
| --- | --- |
| Agent | `bc-f45f1767-cb65-56e8-bd1d-410c0e3916fe` |
| Install commit | `b48cd8002a6d4d509010e04b8a8f1cfcbbdee64a` |
| Tip / packages evidence | `cursor/f14-tip-packages-237f` (PR #326) |
| Config | `X0X_DEV_PADCAL=1` |
| Elite | **PASS** record `5f32e52e93056553` |
| Pro | **DEV OK** gated; not flashed (device Elite) |
| Package source | GitHub prerelease `f14-tip-b48cd800-dev` (not Mac-local rebuild) |
| Publish / nightly / main | not touched from Stage 6 |

## Gate

```
require_gate_pass.sh b48cd800… --config dev --fwsc <attested> --ctl ctl.img \
  --dev-record evidence/f14-tip-packages/{elite,pro}/record.json
→ DEV OK (both)
```

## Install

`fm1_install.py` Elite attested package → exit 0; post-info `SMK-37 Elite_900`; Bluetooth block CRC32 `4a400899` match.

Local record: `~/Library/Application Support/x0x37/unattended-install/records/5f32e52e93056553/`

## Stage 7

`SUCCESSOR-STAGE7.txt` pushed for coordinator CreateAgent. No `CURSOR_API_KEY` on this worker → do not claim Task as Stage 7. This stage does not claim Stage 7.
