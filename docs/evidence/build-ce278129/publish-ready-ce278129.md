# Publish draft for nightly-ce278129-dev

Status: waiting for the owner's go-ahead; not published.

No gh-pages write, no `--push`, and no `--remote` was run. No ledger number was allocated.

## Label and version

- Catalog version key: `nightly-ce278129-dev`
- Label: `UNREVIEWED · DEVICE NOT TESTED`
- `hardware_tested`: false
- `display_version`: `0.0.24.5`

`0.0.24.5` is the `display_version` of `nightly-8be16486-dev` in gh-pages `7c7baecb1be17b860b13e8832882a399679231c1` `versions.json`. That entry says the embedded firmware reports 0.0.24 and that 0.0.24.5 is a catalog label. This candidate's app images and `.fwsc` files do not contain the bytes `0.0.24.5`. `firmware/src/app/version.h` line 7 is the default `X0X_VERSION` macro used by the ABOUT line. The catalog field is still `0.0.24.5`.

## Row to add

| field | value |
| --- | --- |
| `version` | `nightly-ce278129-dev` |
| `label` | `UNREVIEWED · DEVICE NOT TESTED` |
| `status` | `unreviewed` |
| `config` | `X0X_DEV_PADCAL=0` |
| `hardware_tested` | `false` |
| `source_commit` | `ce27812951555ff7ed9694bf99d20a65dd5f410b` |
| `source_tree` | `a917fb6a2c738bb410601bd784278bf35c142f1c` |
| `display_version` | `0.0.24.5` |
| `title` | `UNREVIEWED · DEVICE NOT TESTED` |
| `description` | UNREVIEWED · DEVICE NOT TESTED. Host run measured 391 ok, 6 FAIL, 0 NOT_RUN. FAIL: original-release-ui, smc-ui, offline-sequencer-browser, ui-feedback, ui-glyph-polish, scenario-plock. NOT_RUN: none. package_nightly --check was NOT_RUN: there is no canonical X.Y.Z version for this build, and there is no installer and no package_check digest. The firmware images show "DnBOX DEV". display_version 0.0.24.5 is a carried catalog label only. It is not the firmware's own string. |
| `models.elite.product` | `SMK-37 Elite_900` |
| `models.elite.sha256` | `4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb` |
| `models.elite.size` | `651570` |
| `models.elite.controller_sha256` | `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984` |
| `models.elite.controller_size` | `61552` |
| `models.elite.pkg` | `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/artifacts/nightly-dev/4ca8b7b3f3c10ee5ca46639635cca4da7f13f49791a3b8a9d5d5cf59df2c8deb/x0x37-elite-dev.fwsc` |
| `models.pro.product` | `SMK-37 Pro_900` |
| `models.pro.sha256` | `cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7` |
| `models.pro.size` | `651566` |
| `models.pro.pkg` | `https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/artifacts/nightly-dev/cbb090311d0d9f9df92bfcf99c70c9354dc135a340d7438053d1c20a66b7acf7/x0x37-pro-dev.fwsc` |

## Package identity against the row

| package fact | catalog field | equal? |
| --- | --- | --- |
| `SMK-37 Elite_900` / `SMK-37 Pro_900` | `models.*.product` | equal |
| elite-ble 651570 / `4ca8b7b3…` | `models.elite.size` / `sha256` | equal |
| pro-ble 651566 / `cbb09031…` | `models.pro.size` / `sha256` | equal |
| `ctl.img` 61552 / `a334208b…` | `controller_size` / `controller_sha256` | equal |
| `display_version` `0.0.24.5` | `display_version` | equal across the draft row, the manifest, and `versions-would-be.json` |
| plain elite sha256 `be4d608e…` | `models.elite.sha256` | differs: the row names the bundled `-ble` file. Same size. |
| plain pro sha256 `0773c61a…` | `models.pro.sha256` | differs, same reason |
| `package_nightly --check` digest | `outputs[]` | NOT_RUN. Exact line: `package_nightly: firmware version must be canonical X.Y.Z (components 0..99)`. No digest was printed. |

## What is not touched

All 28 pre-existing entries, including `nightly-8be16486-dev`, stay byte-identical. `catalog-entry-sha256.txt` records `equal` for each. `latest` on the draft points at `nightly-ce278129-dev`.

## Publish commands, not run

```
python3 tools/publish_nightly.py \
  --repo . \
  --elite-record NOT_RUN_no_DEV_CHECKED_record \
  --elite-fwsc ELITE_BLE_FWSC \
  --elite-ctl CTL_IMG \
  --pro-record NOT_RUN_no_DEV_CHECKED_record \
  --pro-fwsc PRO_BLE_FWSC \
  --pro-ctl CTL_IMG \
  --out SCRATCH_PREVIEW_DIR
```

No `--push`. No `--remote`. This command was not executed.
