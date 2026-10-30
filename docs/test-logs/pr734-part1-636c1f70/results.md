# PR #734: Part 1 exact-source host and Elite test evidence

- Repository: gobbajogger/X0X37-for-Loopy-Pro
- Tested integration branch: integration/all-in-one
- Exact head: 636c1f7013a1cf02e34b8d23031086777d898e7b
- Git root tree: 2a6a68b4c7515a757f07af12f2c6f59335b5c7a0
- Restored source check: 4,579/4,579 Git blob hashes and sizes matched independently in each isolated source copy
- Test runner: tests/run_tests.sh, original Git blob df5242cc30a999cd07ac7a4232d7db38e7ff23d2
- Split: first 158 of 316 ordered cases, disjoint from Part 2; original suite code unchanged
- Mac mini, macOS arm64; 3 host test jobs; Python from /Users/mac/.venvs/dnbox-build
- Host process result: host_exit=1
- Host results: **112 PASS / 43 FAIL / 3 NOT_RUN**
- Elite command: BOARD=smk37 MODEL=elite sh build.sh
- Elite result: elite_exit=1
- Source and build logs remain local; only sanitized text evidence is uploaded to GitHub

## Every selected test (original suite order)

| # | Name | Result |
|---:|---|---|
| 1 | `dsp-host-linkage` | PASS |
| 2 | `fastmath` | PASS |
| 3 | `fastmath-ubsan` | PASS |
| 4 | `home-tie-semantics` | PASS |
| 5 | `sequencer` | PASS |
| 6 | `bass-prob-midi` | FAIL |
| 7 | `seq-tb3po-bars` | FAIL |
| 8 | `seq-random-swing` | FAIL |
| 9 | `seq-random-clock-handoff` | FAIL |
| 10 | `seq-long-notes` | PASS |
| 11 | `seq-pingpong-resize` | PASS |
| 12 | `clk-lock` | FAIL |
| 13 | `seq-random-tie-preview` | PASS |
| 14 | `encoder` | PASS |
| 15 | `bass-midi-pending-native` | PASS |
| 16 | `uac` | PASS |
| 17 | `trs` | PASS |
| 18 | `undo` | PASS |
| 19 | `undo-capacity` | PASS |
| 20 | `lz` | PASS |
| 21 | `motion` | PASS |
| 22 | `host-build` | FAIL |
| 23 | `original-release-integration` | PASS |
| 24 | `original-release-ui` | PASS |
| 25 | `original-build` | FAIL |
| 26 | `freeplay-rate-pads` | PASS |
| 27 | `boot-screen` | PASS |
| 28 | `ui-theme` | PASS |
| 29 | `project-protocol` | FAIL |
| 30 | `project-save-preflight` | FAIL |
| 31 | `editor-ota-dispatch` | PASS |
| 32 | `storage-scratch` | FAIL |
| 33 | `project-stage` | FAIL |
| 34 | `project-runtime` | FAIL |
| 35 | `save-note-quiet` | FAIL |
| 36 | `font-pack` | FAIL |
| 37 | `smc-protocol` | PASS |
| 38 | `smc-fader` | PASS |
| 39 | `smc-buttons` | FAIL |
| 40 | `smc-select-mode` | FAIL |
| 41 | `smc-fx` | PASS |
| 42 | `smc-mixer-edges` | PASS |
| 43 | `smc-action-source` | PASS |
| 44 | `smc-fx-caption` | PASS |
| 45 | `smc-fx-preset-preserve` | PASS |
| 46 | `smc-queue` | PASS |
| 47 | `smc-solo-overflow` | PASS |
| 48 | `engine-cq-rec-overflow` | PASS |
| 49 | `engine-cq-isr-preemption` | PASS |
| 50 | `engine-cq-boff-overflow` | PASS |
| 51 | `engine-cq-brk-overflow` | PASS |
| 52 | `engine-cq-bon-overflow` | PASS |
| 53 | `engine-cq-drum-overflow` | PASS |
| 54 | `midi-mq-noteoff-overflow` | PASS |
| 55 | `midi-mq-rt-overflow` | PASS |
| 56 | `midi-overflow-order` | PASS |
| 57 | `midi-out-mq-noteoff-overflow` | PASS |
| 58 | `ble-out-noteoff-overflow` | PASS |
| 59 | `ble-out-rt-overflow` | PASS |
| 60 | `usb-sxq-sticky-apc` | PASS |
| 61 | `smc-host-data7` | PASS |
| 62 | `smc-host-overflow-data7` | PASS |
| 63 | `smc-sender-isolation` | PASS |
| 64 | `smc-ble-payload` | PASS |
| 65 | `smc-link` | PASS |
| 66 | `smc-reconnect-epoch` | PASS |
| 67 | `smc-boot-saved-roles` | FAIL |
| 68 | `ble-central-first-notification` | PASS |
| 69 | `ble-one-peer-command` | PASS |
| 70 | `ble-one-peer-contract` | PASS |
| 71 | `ble-dual-peer` | PASS |
| 72 | `ble-command-credit` | PASS |
| 73 | `ble-controller-heap` | NOT-RUN |
| 74 | `ble-midi-receive` | FAIL |
| 75 | `ble-capture-replay` | FAIL |
| 76 | `ble-acl-start-flags` | PASS |
| 77 | `ble-hci-completed` | PASS |
| 78 | `ble-acl-zero-credit` | PASS |
| 79 | `ble-link-epoch-reset` | PASS |
| 80 | `ble-disconnect-status` | PASS |
| 81 | `smc-reachability` | FAIL |
| 82 | `smc-pickup-acceptance` | PASS |
| 83 | `smc-lock` | FAIL |
| 84 | `smc-lock-beat` | PASS |
| 85 | `smc-multi-peer` | FAIL |
| 86 | `smc-multi-peer-negative` | PASS |
| 87 | `smc-pause-loss` | PASS |
| 88 | `smk-wheel-mode` | FAIL |
| 89 | `miami-table-interpolation` | PASS |
| 90 | `legacy-303-project` | FAIL |
| 91 | `smc-interaction-negative` | FAIL |
| 92 | `corner-coverage` | PASS |
| 93 | `part-order` | PASS |
| 94 | `smc-ui` | PASS |
| 95 | `smc-mode-page` | PASS |
| 96 | `smc-mixer-page` | PASS |
| 97 | `ui-shared-glyph` | PASS |
| 98 | `ui-drum-mix-material` | PASS |
| 99 | `ui-waveform-truth` | FAIL |
| 100 | `smc-ui-bounds` | PASS |
| 101 | `smc-ui-negative` | FAIL |
| 102 | `smc-mapping` | PASS |
| 103 | `smc-drum-mutes` | FAIL |
| 104 | `smc-drum-rs` | FAIL |
| 105 | `smc-performance-abort` | FAIL |
| 106 | `smc-filter` | PASS |
| 107 | `smc-feedback` | PASS |
| 108 | `smc-record-feedback` | FAIL |
| 109 | `smc-part-select` | PASS |
| 110 | `smc-clip-visibility` | PASS |
| 111 | `drum-pan` | PASS |
| 112 | `stereo` | PASS |
| 113 | `master` | PASS |
| 114 | `drum-kit-identity` | FAIL |
| 115 | `drum909-adpcm` | PASS |
| 116 | `drum909` | NOT-RUN |
| 117 | `drum808` | NOT-RUN |
| 118 | `breaks` | PASS |
| 119 | `break-suite-registration` | FAIL |
| 120 | `adaptive-break-dsp` | PASS |
| 121 | `adaptive-break-controls` | PASS |
| 122 | `adaptive-break-storage` | FAIL |
| 123 | `adaptive-break-midi-pair` | PASS |
| 124 | `break-performance` | PASS |
| 125 | `brk-loops-seamless` | PASS |
| 126 | `brk-outline` | PASS |
| 127 | `brk-outline-gate` | PASS |
| 128 | `chicago-fx-bridge` | FAIL |
| 129 | `chicago-fx-runtime` | FAIL |
| 130 | `au-compare-align` | PASS |
| 131 | `au-compare-battery` | PASS |
| 132 | `install-gate-guard` | PASS |
| 133 | `queue-guard` | PASS |
| 134 | `install-guards` | PASS |
| 135 | `install-head` | PASS |
| 136 | `release-tree` | PASS |
| 137 | `prepush-trim` | PASS |
| 138 | `prepush-allow` | PASS |
| 139 | `gate-configs` | PASS |
| 140 | `gate-trap` | FAIL |
| 141 | `quickcheck-plan` | PASS |
| 142 | `host-failure-summary` | PASS |
| 143 | `ctl-compile` | PASS |
| 144 | `host-setup` | FAIL |
| 145 | `gen-nonfont-tables` | FAIL |
| 146 | `stereo-alone-gen` | PASS |
| 147 | `runner-admission` | PASS |
| 148 | `update-names` | FAIL |
| 149 | `gate-reason` | PASS |
| 150 | `validation-inputs` | PASS |
| 151 | `stage-handoff` | PASS |
| 152 | `dev-validate` | PASS |
| 153 | `dev-validate-composed` | FAIL |
| 154 | `nightly-publish` | PASS |
| 155 | `gen-cache` | PASS |
| 156 | `smk-update-contract` | PASS |
| 157 | `gen-order` | FAIL |
| 158 | `quickcheck-devbug` | FAIL |

## Failures

- `bass-prob-midi`
- `seq-tb3po-bars`
- `seq-random-swing`
- `seq-random-clock-handoff`
- `clk-lock`
- `host-build`
- `original-build`
- `project-protocol`
- `project-save-preflight`
- `storage-scratch`
- `project-stage`
- `project-runtime`
- `save-note-quiet`
- `font-pack`
- `smc-buttons`
- `smc-select-mode`
- `smc-boot-saved-roles`
- `ble-midi-receive`
- `ble-capture-replay`
- `smc-reachability`
- `smc-lock`
- `smc-multi-peer`
- `smk-wheel-mode`
- `legacy-303-project`
- `smc-interaction-negative`
- `ui-waveform-truth`
- `smc-ui-negative`
- `smc-drum-mutes`
- `smc-drum-rs`
- `smc-performance-abort`
- `smc-record-feedback`
- `drum-kit-identity`
- `break-suite-registration`
- `adaptive-break-storage`
- `chicago-fx-bridge`
- `chicago-fx-runtime`
- `gate-trap`
- `host-setup`
- `gen-nonfont-tables`
- `update-names`
- `dev-validate-composed`
- `gen-order`
- `quickcheck-devbug`

## Not run / skipped

- `ble-controller-heap`: not-run ble-controller-heap (single-peer recovery; B two-link heap assumption removed)
- `drum909`: not-run drum909 (../schwung-9W9/src/dsp/er99_engine.c)
- `drum808`: not-run drum808 (../schwung-8W8/src/dsp/sc808_engine.cpp)

## Elite safety check

```text
font L: BarlowSemiCondensed-Bold.ttf 31px h 32, digit adv 17, 3615 B
font set barlow: 23190 B (approx)
gen_drum_samples: 110549 frames (55275 bytes of ADPCM, 221098 as int16) -> x0x_drum_samples.h; tanh 1026 packed 4104->3046 B + 19 pot curves -> x0x_drum_tables.h
break bank: none (factory loops are an optional install)
loader: image 8412 B at 0x1c0a800, bss end 0x1c0f700
loader: ota.bin 7341 B
  ok    .ram_text: 844 insns, no calls
  ok    app code area 552672 B, 100736 B free (Bluetooth block at app.bin 0x86ee0)
  ok    no soft-double routines (single-precision FPU only)
  ok    image 451936 B; RAM .data+.bss 94960 B of 98304; pool 333772 B of 344064
  FAIL  src/app/engine.c:48: register/window address outside hal/ (0x10000u); add a hal/ helper
build: checks failed
```

