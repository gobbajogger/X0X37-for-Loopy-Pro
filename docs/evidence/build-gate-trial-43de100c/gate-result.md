# Gate result for tree 97f210b2c48d8c5a3d2216d40030247911c894bb

This file uses ASD Simplified Technical English.

Result: TRIAL FAIL (a trial run with --trial: not a release PASS)
Reason: Elite build (dev): 

- Purpose: owner-validation
- Run reason: informational trial of 43de100c on a cloud VM, not a release
- Commit: 43de100c83d845409bb801ecf29238a78f179aed
- Tree (SHA-1 of git ls-tree -r without *.md): 97f210b2c48d8c5a3d2216d40030247911c894bb
- Git tree (git rev-parse <commit>^{tree}): 4993a90ae993da7d68ece0d352c88b27244b327e
- References: pinned schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d. Found: schwung-9W9=10cbe5c schwung-8W8=94aa271 schwung-303=7680578 schwung-tb3po=f49d81d.
- Private HOME and TMPDIR: /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1142-4199.logs/home, /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1142-4199.logs/tmp (rel: /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1142-4199.logs/home-rel, /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1142-4199.logs/tmp-rel)
- Configs: dev (X0X_DEV_PADCAL=1, worktree /home/ubuntu/.cache/x0x37-gate/43de100) and rel (X0X_DEV_PADCAL=0, the release build, worktree /home/ubuntu/.cache/x0x37-gate/43de100.rel). The two must pass.
- Config CRCs: dev elite , ble , code free ? B, pool 0 B; rel elite , ble , code free ? B, pool 0 B
- Validation inputs: 08b1ef72e7d9fdf0031e20e225b433682c0699ffb4a9250c08802a7cf445b5ed
- Build variables of the caller (removed): JIELI_DOCKER=0
- Time: 2026-11-05 11:42 UTC. The gate took 176 s.
- Worktree: /home/ubuntu/.cache/x0x37-gate/43de100. Logs: /home/ubuntu/.cache/x0x37-gate/43de100-20261105-1142-4199.logs.

## Builds

- Elite build: 
- Pro build: 
- Bluetooth image: ble/ctl/out/ctl.img: 61552 B of 65536, crc32 564a8f55, RAM 0x1c01800..0x1c07ee0 (26336 B of 26368)

## Sizes

| Build | App code free | App RAM (.data+.bss) | Pool (the headroom limit is 8192 B) |
|---|---|---|---|
| elite | ? B | ? of ? B, 0 B free | ? of ? B, 0 B headroom |
| pro | ? B | ? of ? B, 0 B free | ? of ? B, 0 B headroom |
| elite rel | ? B | ? of ? B, 0 B free | ? of ? B, 0 B headroom |
| pro rel | ? B | ? of ? B, 0 B free | ? of ? B, 0 B headroom |

| Item | Value |
|---|---|
| X0X_RAM_CANARY | 0 (default in firmware/src/ble.c) |
| ctl.img | 61552 of 65536 B, 3984 B free |
| Core RAM | 26336 of 26368 B, 32 B free |

## Tests

- Time scale (X0X_TEST_TIME_SCALE): 1. The gate always uses 1: the real waits of the updater.
- tests/run_tests.sh (X0X_TEST_JOBS=8): 248 ok, 58 FAIL, 1 skip. Exit code 1.
- tests/run_tests.sh, rel config (X0X_TEST_JOBS=8): 248 ok, 58 FAIL, 1 skip. Exit code 1. Scenarios: 0 ok. Tests that did not run: ble-package chicago-presentation chicago-readout copy-semantics ext-midi-rec-smk font-pack gen-order home-overview host-build matrix-knob-caption perf-scratch preset-navigation-noop project-runtime rec-live-smk save-note-quiet scenario-arrows scenario-autosave scenario-bleclock scenario-blenotes scenario-brkslice scenario-brkslice_mode_toggle scenario-brkslice_trs_overlap scenario-clip-arrow-menu scenario-clklock scenario-clkmove scenario-clocksrc scenario-drum_bass_ch_consume scenario-edit_midi_rec_guard scenario-edit_oct_no_hidden scenario-extclock scenario-freeplay scenario-getting_started scenario-groove scenario-guard scenario-help scenario-jam scenario-keysound scenario-knobs scenario-midiowner scenario-migrate scenario-migrate0102 scenario-patterns scenario-perf scenario-perfsave scenario-plock scenario-prob scenario-probcond scenario-probreset scenario-roll_continuation scenario-roll_short_tie scenario-savefeedback scenario-savewait scenario-showcase scenario-slices scenario-song scenario-song_live_mute_save scenario-songrep scenario-sound_matrix_chicago scenario-sound_matrix_chicago_idle scenario-sound_matrix_drum scenario-sound_matrix_lock scenario-sound_matrix_miami scenario-sound_page_split scenario-step303 scenario-steps64 scenario-stereo scenario-tb3po scenario-tour scenario-trackmute scenario-undo scenario-upload scenario-upload44 scenario-usbnotes smc-drum-mutes smc-drum-rs smc-performance-abort smc-reachability smc-record-feedback smk-build-package smk-build-package-elite smk-clip-chord-display smk-ota-entry smk-ota-entry-model1 smk-ota-entry-model2 smk-package smk-update-loader song-actions sound-layout sound-matrix ui-303-entry ui-feedback.
- Gate lock: /tmp/x0x37-gate.lock: waited 0 s, held 170 s.
- The script knows 397 tests. Tests that did not run: ble-package chicago-presentation chicago-readout copy-semantics ext-midi-rec-smk font-pack gen-order home-overview host-build matrix-knob-caption perf-scratch preset-navigation-noop project-runtime rec-live-smk save-note-quiet scenario-arrows scenario-autosave scenario-bleclock scenario-blenotes scenario-brkslice scenario-brkslice_mode_toggle scenario-brkslice_trs_overlap scenario-clip-arrow-menu scenario-clklock scenario-clkmove scenario-clocksrc scenario-drum_bass_ch_consume scenario-edit_midi_rec_guard scenario-edit_oct_no_hidden scenario-extclock scenario-freeplay scenario-getting_started scenario-groove scenario-guard scenario-help scenario-jam scenario-keysound scenario-knobs scenario-midiowner scenario-migrate scenario-migrate0102 scenario-patterns scenario-perf scenario-perfsave scenario-plock scenario-prob scenario-probcond scenario-probreset scenario-roll_continuation scenario-roll_short_tie scenario-savefeedback scenario-savewait scenario-showcase scenario-slices scenario-song scenario-song_live_mute_save scenario-songrep scenario-sound_matrix_chicago scenario-sound_matrix_chicago_idle scenario-sound_matrix_drum scenario-sound_matrix_lock scenario-sound_matrix_miami scenario-sound_page_split scenario-step303 scenario-steps64 scenario-stereo scenario-tb3po scenario-tour scenario-trackmute scenario-undo scenario-upload scenario-upload44 scenario-usbnotes smc-drum-mutes smc-drum-rs smc-performance-abort smc-reachability smc-record-feedback smk-build-package smk-build-package-elite smk-clip-chord-display smk-ota-entry smk-ota-entry-model1 smk-ota-entry-model2 smk-package smk-update-loader song-actions sound-layout sound-matrix ui-303-entry ui-feedback.
- Scenarios: 0 ok.
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
```
