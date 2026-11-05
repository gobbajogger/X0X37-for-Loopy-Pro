# Gate result for tree 97f210b2c48d8c5a3d2216d40030247911c894bb

This file uses ASD Simplified Technical English.

Result: TRIAL FAIL (a trial run with --trial: not a release PASS)
Reason: tests (dev): 16 failed (original-release-ui (see build/host/original-release-ui.log))

- Purpose: owner-validation
- Run reason: informational trial 2 of 43de100c on a cloud VM after environment fixes, not a release
- Commit: 43de100c83d845409bb801ecf29238a78f179aed
- Tree (SHA-1 of git ls-tree -r without *.md): 97f210b2c48d8c5a3d2216d40030247911c894bb
- Git tree (git rev-parse <commit>^{tree}): 4993a90ae993da7d68ece0d352c88b27244b327e
- References: pinned schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d. Found: schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d.
- Private HOME and TMPDIR: /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1154-46558.logs/home, /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1154-46558.logs/tmp (rel: /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1154-46558.logs/home-rel, /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1154-46558.logs/tmp-rel)
- Configs: dev (X0X_DEV_PADCAL=1, worktree /home/ubuntu/.cache/x0x37-gate/43de100) and rel (X0X_DEV_PADCAL=0, the release build, worktree /home/ubuntu/.cache/x0x37-gate/43de100.rel). The two must pass.
- Config CRCs: dev elite 35cb6d2d, ble ee944478, code free 36664 B, pool 8200 B; rel elite e072beb3, ble 3b2d97e6, code free 38664 B, pool 8200 B
- Validation inputs: 0f0d04451ca21bc8cc0d3c1a2e867c97fcd14db94c050bd492a4a178c8846003
- Build variables of the caller (removed): JIELI_DOCKER=0
- Time: 2026-11-05 11:54 UTC. The gate took 1363 s.
- Worktree: /home/ubuntu/.cache/x0x37-gate/43de100. Logs: /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1154-46558.logs.

## Builds

- Elite build: package  build/x0x-smk37-elite.fwsc  651570 B, identity SMK-37 Elite_900, board smk37
- Pro build: package  build/x0x-smk37.fwsc  651566 B, identity SMK-37 Pro_900, board smk37
- Bluetooth image: ble/ctl/out/ctl.img: 61552 B of 65536, crc32 564a8f55, RAM 0x1c01800..0x1c07ee0 (26336 B of 26368)

## Sizes

| Build | App code free | App RAM (.data+.bss) | Pool (the headroom limit is 8192 B) |
|---|---|---|---|
| elite | 36664 B | 97744 of 98304 B, 560 B free | 335864 of 344064 B, 8200 B headroom |
| pro | 36680 B | 97744 of 98304 B, 560 B free | 335864 of 344064 B, 8200 B headroom |
| elite rel | 38664 B | 97328 of 98304 B, 976 B free | 335864 of 344064 B, 8200 B headroom |
| pro rel | 38680 B | 97328 of 98304 B, 976 B free | 335864 of 344064 B, 8200 B headroom |

| Item | Value |
|---|---|
| X0X_RAM_CANARY | 0 (default in firmware/src/ble.c) |
| ctl.img | 61552 of 65536 B, 3984 B free |
| Core RAM | 26336 of 26368 B, 32 B free |

## Tests

- Time scale (X0X_TEST_TIME_SCALE): 1. The gate always uses 1: the real waits of the updater.
- tests/run_tests.sh (X0X_TEST_JOBS=8): 381 ok, 16 FAIL, 0 skip. Exit code 1.
- tests/run_tests.sh, rel config (X0X_TEST_JOBS=8): 381 ok, 16 FAIL, 0 skip. Exit code 1. Scenarios: 57 ok. Tests that did not run: none.
- Gate lock: /tmp/x0x37-gate.lock: waited 0 s, held 1342 s.
- The script knows 397 tests. Tests that did not run: none.
- Scenarios: 57 ok.
- ble_test: ble_test: 5153 checks, 0 failed
- smk_clip_keys_test: smk_clip_keys_test: ok

## Flash map

```
  0x000000-0x003fff   16 KiB  head
  0x004120-0x08afff  539 KiB  app code
  0x08b000-0x09afff   64 KiB  Bluetooth block
  0x09b000-0x09cfff    8 KiB  app slot end and cfg
  0x09d000-0x0affff   76 KiB  store
  0x0b0000-0x0bafff   44 KiB  VM spare
  0x0bb000-0x0bffff   20 KiB  OTA
  0x0c0000-0x0c1fff    8 KiB  BTIF and USRTRIM
  0x0c2000-0x0f3fff  200 KiB  slots 1 and 2
  0x0f4000-0x0fdfff   40 KiB  USR
  0x0ff000-0x0fffff    4 KiB  key_mac
owner tag (ST_OWNER_SECTOR): 0xaf000
VM spare: 0xb0000-0xbafff, 44 KiB
flashmap: PASS
```

## CRC manifest

```
ctl.img header crc32 564a8f55, image_size 61552 B
ctl.img file crc32 9f78fdb8, 61552 B
build/smk37-elite/loader/ota.bin crc32 ec25eef3, 7454 B
build/smk37-elite/x0x.bin crc32 36497f1a, 516008 B
build/smk37/loader/ota.bin crc32 5b379f7a, 7450 B
build/smk37/x0x.bin crc32 115573ae, 515992 B
build/x0x-smk37-elite-ble.fwsc crc32 ee944478, 651570 B
build/x0x-smk37-elite.fwsc crc32 35cb6d2d, 651570 B
build/x0x-smk37.fwsc crc32 719ddcc2, 651566 B
rel/build/smk37-elite/loader/ota.bin crc32 ec25eef3, 7454 B
rel/build/smk37-elite/x0x.bin crc32 25e58760, 514008 B
rel/build/smk37/loader/ota.bin crc32 5b379f7a, 7450 B
rel/build/smk37/x0x.bin crc32 2f64fba8, 513992 B
rel/build/x0x-smk37-elite-ble.fwsc crc32 3b2d97e6, 651570 B
rel/build/x0x-smk37-elite.fwsc crc32 e072beb3, 651570 B
rel/build/x0x-smk37.fwsc crc32 76e43cd3, 651566 B
```
