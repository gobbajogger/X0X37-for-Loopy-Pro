# Pattern notification — implemented

Owner approved without preview and explicitly removed the visual approval wait.
RW/FF (0x5B/ 0x5C) never open a popup. The existing top notification bar renders
NEXT, the existing HOME pattern glyph, an arrow and the cued number for 1000 ms.
A cancelled cue shows the empty glyph and --. PLAY and MMC Play/Stop remain
transport-only. An existing popup stays in place; keyboard navigation is unchanged.

Existing ui_say, draw_header, HOME pattern projection/glyph and popup drawing are
reused. No new renderer, screen or persistent state. smc_connected now iterates
only the configured SMC_PEERS, fixing the default one-peer out-of-bounds read.

| Real 240x240 host framebuffer | Before | After |
|---|---|---|
| Keyboard screen | [Before](before-pattern-from-keyboard.png) | [After](after-pattern-from-keyboard.png) |
| Existing popup | [Before](before-pattern-over-popup.png) | [After](after-pattern-over-popup.png) |

Each after image differs from its immediate keyboard/popup predecessor only in
the top 20 pixel rows. After notice expiry, the retained popup is byte-identical
to its earlier frame. The capture seeds real notes in the cued pattern so the
reused HOME glyph has visible content; no mock screen is painted.

Fresh SMC/mixer run: 47 PASS / 0 FAIL / 0 selected NOT_RUN, including all nine
previous failures. Popup 4958 checks PASS; glyph pixels and 1000 ms timeout each
had a separate RED reproduction. The connected-status sanitizer first reproduced
index 1 outside a one-element state array; O0/O2 UBSan/bounds now pass 22 cases
across one- and two-peer builds and both central-role states.

Fresh Elite/Pro native builds PASS with SDK V1.2.1, X0X_DEV_PADCAL=0. Static RAM
free 2432 B each (base 2352 B), pool free 8536 B. No flashing or firmware binaries.
Full repository host suite and physical hardware tests remain NOT_RUN.

Reproduce actual framebuffer dumps through the existing host harness:

```sh
SMC_UI_TEST_FILE=docs/engineering-delivery/smc-dedicated-popup-023/pattern-notice-proposal/capture.c SMC_UI_OUT=build/host/pattern-notice-capture sh tests/host/smc_ui_test.sh
```

Source/test hashes and full selector results: [verification.json](verification.json).
