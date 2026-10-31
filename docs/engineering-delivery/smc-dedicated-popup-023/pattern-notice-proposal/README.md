# Pattern notification proposal — awaiting owner visual approval

Rewind/Forward cue the pattern and use the existing 1.3-second top notification
bar, `NEXT: PAT 5`; a cleared cue shows `NEXT: PAT --`. They create no popup and
retain an open popup. PLAY and MMC Play/Stop remain transport-only.

No new renderer, screen or persistent state. Existing ui_say, draw_header and
popup drawing are reused. Keyboard view/part/mixer selection/pages stay unchanged.
The existing header toast is drawn over an open popup only for pattern notices.

All four PNGs are actual 240x240 host framebuffer dumps, losslessly compressed:

| Case | Before (a74949ab) | After (local proposal) |
|---|---|---|
| Keyboard screen | [Before](before-pattern-from-keyboard.png) | [After](after-pattern-from-keyboard.png) |
| Existing popup | [Before](before-pattern-over-popup.png) | [After](after-pattern-over-popup.png) |

The after captures change only the top 20 pixel rows; pixels below the header
match the same keyboard/popup immediately before pattern input byte for byte.
Popup tests: 4938 checks PASS, including five input routes, locked/unlocked
contexts, popup preservation, cue cancellation, bounds, notification expiry and
keyboard invariance. Action-source: 420 checks PASS. Button fixture now extracts
real text writers and checks the cue/cancel notice payload.

Elite and Pro native builds PASS, SDK V1.2.1, X0X_DEV_PADCAL=0. Each has 2432 B
free static RAM (base 2352 B), pool free 8536 B. App: Elite 497804 B, Pro 497788 B.
The new pattern string uses stack/UI notice storage; no new persistent state.

Latest SMC/mixer selector results: **47 PASS / 0 FAIL / 0 selected NOT_RUN**.
The full selected runner first returned 46 PASS and one button-fixture compile
failure (missing text-writer dependencies). That fixture now extracts the real
helpers; its registered rerun passes. Unaffected selector proof is retained.
Four independent code reviewers and the visual proposal review found no defects.
See [verification.json](verification.json) for candidate input hashes and results.
Full repository host suite and physical hardware tests NOT_RUN.
This is a proposal for owner approval, not permission to merge or flash.

Reproduce after applying the reviewed candidate (or run against the baseline
checkout for the before frames):

```sh
SMC_UI_TEST_FILE=docs/engineering-delivery/smc-dedicated-popup-023/pattern-notice-proposal/capture.c SMC_UI_OUT=build/host/pattern-notice-capture sh tests/host/smc_ui_test.sh
```

`capture.c` reuses the existing production host harness and png_write path;
it does not paint a mock screen. The current PR firmware remains unchanged
until owner approval of the proposal. The candidate stays local and is identified
by its source/test SHA-256 hashes in the proposal verification receipt.
