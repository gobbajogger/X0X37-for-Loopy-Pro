# Publish draft for nightly-ce278129-dev

Status: waiting for the owner's go-ahead; not published.

This file records the row that would be added. It was not sent anywhere. No `--push` ran. No remote was named. No gh-pages branch was updated. No ledger number was allocated.

## Label and version

- Catalog version key: `nightly-ce278129-dev`
- Ledger number: none
- `display_version`: `DEV`

`display_version` is the firmware's own version string. `firmware/src/app/version.h` line 7 is `#define X0X_VERSION "DEV"`. `firmware/src/app/ui.c` places that macro in the ABOUT line `DnBOX ` + version + `  MEM `. Both built app images contain the bytes `DnBOX DEV  MEM ` and `X0X DEV`.

The expected catalog display `0.0.24.5` is not that string. The live entry `nightly-8be16486-dev` uses `0.0.24.5`, and its description says the embedded firmware reports 0.0.24 and that 0.0.24.5 is a catalog label only. This candidate's packages disagree with `0.0.24.5`. The draft keeps `DEV` so the row matches the package that was actually built.

## Row to add

Live schema keys, taken from `nightly-8be16486-dev` in the HTTP 200 file `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/versions.json`:

| field | value |
| --- | --- |
| `version` | `nightly-ce278129-dev` |
| `label` | `QA IN PROGRESS / DEVICE NOT TESTED` |
| `status` | `unreviewed` |
| `config` | `X0X_DEV_PADCAL=0` |
| `hardware_tested` | `false` |
| `source_commit` | `ce27812951555ff7ed9694bf99d20a65dd5f410b` |
| `source_tree` | `a917fb6a2c738bb410601bd784278bf35c142f1c` |
| `display_version` | `DEV` |
| `title` | `QA IN PROGRESS / DEVICE NOT TESTED` |
| `description` | `QA IN PROGRESS / DEVICE NOT TESTED.` The built ABOUT string is `DEV`. Host FAIL and NOT_RUN names for this run were not known yet because the suite was still running. `package_nightly --check` produced no digest. No ledger number. |
| `evidence_url` | `https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/ce27812951555ff7ed9694bf99d20a65dd5f410b` |
| `models.elite.product` | `SMK-37 Elite_900` |
| `models.elite.sha256` | `4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb` |
| `models.elite.size` | `651570` |
| `models.elite.controller_sha256` | `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` |
| `models.elite.controller_strategy` | `bundled` |
| `models.elite.controller_size` | `61552` |
| `models.elite.pkg` | `/home/ubuntu/x0x-scratch-ce278129/pkg/x0x-smk37-elite-ble.fwsc` |
| `models.pro.product` | `SMK-37 Pro_900` |
| `models.pro.sha256` | `cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7` |
| `models.pro.size` | `651566` |
| `models.pro.controller_sha256` | `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` |
| `models.pro.controller_strategy` | `bundled` |
| `models.pro.controller_size` | `61552` |
| `models.pro.pkg` | `/home/ubuntu/x0x-scratch-ce278129/pkg/x0x-smk37-ble.fwsc` |

The live file has no `notes` field. The requested text `QA IN PROGRESS / DEVICE NOT TESTED` is stored in `label`, `title`, and the start of `description`.

`status` is `unreviewed` because this build is not a DEV CHECKED record. The live rows use `dev-checked`. Putting that word here would claim a check this run did not perform.

A public download URL is **NOT_RUN**. `pkg` is the local scratch file so the hash can be checked against a real file. Those paths are not on the pages site.

## What is not touched

The live `versions` array has 28 entries. `nightly-8be16486-dev` is one of them. The other 27 stay as they are. There are not 28 further entries besides `nightly-8be16486-dev`.

`catalog-entry-sha256.txt` shows `equal` for all 28 pre-existing entries, including `nightly-8be16486-dev`. `versions-json.diff` changes only `latest` and inserts the new object. `history` stays `[]`. No older `pkg`, sha256, or size moves.

`latest` on the draft points at `nightly-ce278129-dev`.

## Package identity against the row

| package fact | catalog field | equal? |
| --- | --- | --- |
| product string `SMK-37 Elite_900` / `SMK-37 Pro_900` from the build log | `models.*.product` | equal |
| elite-ble size 651570 and sha256 `4ca8b7b3…` | `models.elite.size` / `sha256` | equal |
| pro-ble size 651566 and sha256 `cbb09031…` | `models.pro.size` / `sha256` | equal |
| `ctl.img` size 61552 and sha256 `a334208b…` | `controller_size` / `controller_sha256` | equal |
| ABOUT string `DEV` | `display_version` | equal |
| expected `0.0.24.5` | `display_version` | disagree |
| plain elite sha256 `be4d608e…` | `models.elite.sha256` | differs, because the row names the `-ble` file. Same size. The controller block is the difference. |
| plain pro sha256 `0773c61a…` | `models.pro.sha256` | differs, same reason |

## package_check against outputs[]

`package_nightly --check` sha256: **NOT_RUN**. Exact line: `package_nightly: firmware version must be canonical X.Y.Z (components 0..99)`. No digest was printed, so it neither equals nor differs from `outputs[]`. The image, if a canonical argument is forced through the parser, stops at `NIGHTLY image contains DnBOX DEV`. The five `outputs[]` hashes are unchanged and match 43de100c.

## Publish commands, not run

These lines were not executed.

```
python3 tools/publish_nightly.py \
  --repo . \
  --elite-record NOT_RUN_no_DEV_CHECKED_record \
  --elite-fwsc /home/ubuntu/x0x-scratch-ce278129/pkg/x0x-smk37-elite-ble.fwsc \
  --elite-ctl /home/ubuntu/x0x-scratch-ce278129/pkg/ctl.img \
  --pro-record NOT_RUN_no_DEV_CHECKED_record \
  --pro-fwsc /home/ubuntu/x0x-scratch-ce278129/pkg/x0x-smk37-ble.fwsc \
  --pro-ctl /home/ubuntu/x0x-scratch-ce278129/pkg/ctl.img \
  --out /home/ubuntu/x0x-scratch-ce278129/nightly-preview
```

No `--push`. No `--remote`. The tool's own `--push` path refuses a Git publish. This unreviewed build has no DEV CHECKED record, so the command could not validate a preview even if it were started. It was not started.
