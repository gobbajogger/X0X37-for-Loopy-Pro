# PR #736: preserve the original mixer FX UI

Status: corrected against original PR #688; source changes frozen pending complete qualification.

## Exact source and integration constraints

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Original feature: closed/unmerged PR #688 at `7b2d1726f3515b319d5edcab34529007d33ab3a8`.
- Corrected PR: #736, `chat/mixer-fx-page-port`, input HEAD `2c8fb7d5f975958bd0ef8c41b4ff44e39d5b0c6b`.
- Renderer baseline: live target `integration/all-in-one@a73882485f867d3ffeaea2c872f312fe4319295a`.
- Source read through the GitHub tree/blob API; both baseline and candidate archives fetched through the authenticated GitHub API, never git network.
- Scope: retain only the #688 mixer/FX controls and captions, adapted to the already-integrated multi-peer and UI behavior. No other new screen/preview or new feature. Do not replace integrated `ui.c` with the historical implementation.
- Retain peer-local timeout/context correctness and current source protections; do not change Chicago/Miami audio, project revision 13, `.github/`, protected flash or other branches.

## Verified #688 UI parity

`firmware/src/app/smc_mixer.c` at #688 Git blob `d3c5c7ad60e08aaa1db8f24227e6a6d34c30ee0a` already contains:
- encoder-bank captions, including `CUT+FX` and `LOCKED BANK`;
- colored fader destination travel, without replacing the physical pickup marker;
- eight-strip screen, FX captions, and prior UI behavior.

The present #736 port retains those existing features; do **not** remove them merely because they appeared in the #736 diff relative to a newer integration base. No unrelated new UI is permitted.

## Real framebuffer evidence

The initial SVG proposals have been **deleted**. Replacements are paired PNG captures from the actual `smc_ui_draw()` implementation, not hand-illustrated mockups: `mixer-fx-home-framebuffer-before-after.png` and `mixer-fx-page-framebuffer-before-after.png`. The renderer uses `tests/host/smc_fx_framebuffer_capture.c`, included production engine/UI code and the original `smc_ui_test.sh` build path. See `mixer-fx-preview-review.md` for input SHAs, exact commands, renderer evidence, pixel comparisons and the displayed images. The only added text is the BEFORE/AFTER label **outside** the raw 240 × 240 framebuffer images.

## Qualification and delivery

Required before integration: full host suite plus both `BOARD=smk37 MODEL=elite` and `MODEL=pro` native firmware builds on verified full SDK V1.2.1, with 0 failures and 0 skips for the owner merge gate. Record actual PASS/FAIL/NOT_RUN results, not assumptions. Keep #736 a draft and #734 frozen; do not merge to integration or change distribution or devices.

All changes to this task stay on `chat/mixer-fx-page-port`; GitHub commits have no custom author fields.
