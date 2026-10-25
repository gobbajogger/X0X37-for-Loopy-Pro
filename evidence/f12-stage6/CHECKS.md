# Stage 6 install — F12 BF-008 / de11b6d7

| Field | Value |
| --- | --- |
| Result | **PASS** |
| Record id | `e88f1ce2eb88ae4a` |
| Time (UTC) | 2026-10-25T18:31:40Z → 2026-10-25T18:33:10Z |
| Validated source SHA | `de11b6d7e3ac3322d2db689675d9ed27ed667785` |
| Branch (prompt) | `cursor/f12-stage5-bf008-1207` |
| Evidence commit | `0ebd4856896ea1631c1e3aa5f5ec356775a0270c` |
| Model | SMK-37 Elite_900 |
| Port | X0X FM-1 (rtmidi / mido) |
| Package sha256 | `829bb6da6eb0042af0731a236d6238e50cc13581ce059ebbe02882534a94b056` |
| Controller sha256 | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Gate | `DEV OK de11b6d` via `require_gate_pass.sh` + f12-stage4 elite record |
| Installer exit | 0 |
| Flashed | true |
| Data preservation | **ok** (`readback_ok` true; slot map unchanged) |
| Controller readback | **ok** |
| Post-install diag | `ble_autotest --diag-only` status **READY** |
| Hands-on controls | NOT RUN (needs person at keyboard) |
| Path | Mac LaunchAgent `com.x0x37.unattended-install` / inbox `LOCAL-INSTALL.json` |
| Live page | `nightly-de11b6d7-dev` LIVE_PAGE_MATCH confirmed |

## Local tooling note

Prior F11/F10 installs with matching blank package bytes flashed successfully but recorded `INSTALL CHECK FAILED` because `diag_report` called `json.loads` on `ble_autotest --diag-only` stdout that appends human INLOG/PADS text after the JSON object (`ready` stayed false). The Mac LaunchAgent script was patched locally to parse the first JSON object only; BF-008 Stage 6 then **PASS**.

## Artifacts on this Mac

- `/Users/mac/Library/Application Support/x0x37/unattended-install/records/e88f1ce2eb88ae4a/`
- Inbox prompt role `stage6-install` (F12 git `stage6-install-prompt.json` used wrong `kind` and lacked `branch`/`role`, so LaunchAgent had refused remote tips)

Production `main` not moved by this job (hardware validation rule may allow later FF of matching bytes; not done here).
