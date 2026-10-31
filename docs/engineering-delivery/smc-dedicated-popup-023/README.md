# Existing SMC popup

SMC activity displays the existing `smc_ui_draw_peer` popup and expires over the
keyboard screen. No new screen or renderer. Each peer owns its routing context.
Normal SMC navigation leaves keyboard view/part/mixer selection/pages unchanged.
Explicit dialog answers and Help dismissal retain their keyboard-owned actions.

Outside keyboard clip stages, note5 opens the SMC main-parts mixer (MIX page0);
in a clip stage it is the SMC GLOBAL/BACK shortcut. Note6 is HOME/SONG and dismisses
Help. Inside a confirmation, note5 accepts and note6 cancels. Panel HOME remains
an FX modifier. SCALE/CHORD/ARP fields remain keyboard-owned. Lock captures targets;
SMC BANK/PAGE controls still retarget within that context, and unlock keeps its page.

| Model | Static RAM used | Free RAM | Base free | Pool free | App bytes |
|---|---:|---:|---:|---:|---:|
| Elite | 95872 B | 2432 B | 2352 B | 8536 B | 497692 B |
| Pro | 95872 B | 2432 B | 2352 B | 8536 B | 497676 B |

Both native builds PASS, SDK V1.2.1, `X0X_DEV_PADCAL=0`. The bounded descriptor
cache and captured context cost 64 B versus the previous PR head (2496 B free),
while retaining 80 B more free RAM than 0.0.23. Firmware binaries are not included.

SMC/mixer aggregate: **38 PASS / 9 FAIL / 0 selected NOT_RUN**, 47 selectors.
[Results and remaining failures](TESTS.md). Popup 3718 checks, engine 21, core audit 25,
held-step/song contracts 12, mixer pages 485, peer context 30 and fourteen mutation
checks PASS. Full host suite and device validation NOT_RUN. This remains DRAFT.

[Before/after contact sheet](framebuffer/before-after.png); each individual PNG in
[framebuffer/](framebuffer/) is an actual 240x240 host framebuffer dump. The three
`before-audit-*` images are from the previous PR head. `controlled-before.png` and
`controlled-dismiss.png` are byte-identical with no musical edit. The 1-16 header
and orange markers in the edited example come from length 16→17 and unsaved state,
not a keyboard-header change. Grid values omit units consistently; the large
readout retains units. Visual approval is required before merge.

Provenance: restored guard/modal/HOME fragments from base `bb190749` already in
branch ancestry; retained existing encoder-bank behavior. `git cherry` checked
base ancestry; no whole commits transplanted because their keyboard-page writes
would break isolation. No #788 code, FM-1/main_fm1.c, bootloader/row 73, row120,
BTIF/USRTRIM/efuse/key_mac, Miami ADSR, main, .github/Actions or device writes.

Reproduce: `X0X_DEV_PADCAL=0 python3 tools/build.py --board smk37 --model elite`
(and `--model pro`), with SDK/toolchain configured. Capture using
`SMC_UI_SHOTS=<existing-directory> sh tests/host/smc_ui_popup_test.sh`.
