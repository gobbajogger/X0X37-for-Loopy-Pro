# Existing SMC popup wiring

SMC activity now displays the existing `smc_ui_draw_peer` overlay and lets it
expire over the keyboard screen. The owner withdrew the proposed new screens;
this implementation adds no screen or renderer. Base: `build/0.0.23`,
`bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2` (PR #785). Related: #781; #788 is excluded.

Each SMC peer owns its routing context. Keyboard descriptor definitions are
queried through the existing descriptor emitter without touching its page table.
The old page-peek save/write/restore path and keyboard menu dispatch are removed.
SMC has no references to `ui.view`, `ui.part`, `ui.mixsel` or keyboard pages.
Sound-bank changes refresh descriptor data without clamping the remembered
keyboard page; physical keyboard input later accepts its effective page.
Shared musical parameters, transport and record arming remain shared. SMC SELECT
does not retarget notes played on the keyboard; its REC lamps use its own context.

## Native builds

Both real SMK-37 native builds passed using SDK V1.2.1 and `X0X_DEV_PADCAL=0`.

| Model | Static RAM used | Free RAM | Base free | Pool free | App bytes |
|---|---:|---:|---:|---:|---:|
| Elite | 95808 B | 2496 B | 2352 B | 8536 B | 497260 B |
| Pro | 95808 B | 2496 B | 2352 B | 8536 B | 497260 B |

Free RAM increases by 144 B. No firmware binaries are included.
Toolchain clang SHA-256: `c45189ee624de0430deb0db02e6dce171fb5538e7fe323e3964dc8ac6236240f`.
Build command (with local toolchain/SDK environment configured):
`X0X_DEV_PADCAL=0 python3 tools/build.py --board smk37 --model elite`
and the same command with `--model pro`.

## Verification

- Production queue/dispatcher/popup regression: PASS, 3723 checks.
- Deferred engine refresh and stale keyboard pages: PASS, 14 checks.
- Mixer edges: PASS, 1281 checks. Full selected SMC results: [TESTS.md](TESTS.md).
- Source audit finds no protected keyboard navigation references or keyboard
  dispatch calls in `smc_mixer.c`; no transient page save/write/restore remains.
- Compound Engineering review completed six passes and four validated P1s.
  Locked drum recapture, Help musical actions, deferred keyboard page clamping,
  and stale keyboard engine guards each failed before their fix and pass now.
  Locked engine refs also retain bank identity and release their edit holds.
- This remains a draft; failing legacy tests are recorded rather than called PASS.
  Full host suite and hardware/device validation were not performed.

## Real framebuffer captures

[Before/after contact sheet](framebuffer/before-after.png).
`before-smc-menu-switch.png` is the actual base framebuffer. The remaining PNGs
come from the final production renderer and queue/dispatcher host harness:
`before-keyboard.png`, `after-fader-popup.png`, `after-knob-popup.png`,
`after-mute-popup.png`, `after-dismiss-keyboard.png`. PNG compression is lossless.
Musical edits can change displayed parameter values while navigation stays fixed.

Reproduce final captures:
```sh
SMC_UI_OUT=build/host/smc-popup sh tests/host/smc_ui_popup_test.sh
SMC_UI_SHOTS=build/host/smc-popup build/host/smc-popup/test
sh tests/host/smc_popup_engine_test.sh
```

No main/nightly merge, `.github`/Actions changes, flashing, FM-1 build or device
operation. Exact commit author and committer use the owner-requested identity.
