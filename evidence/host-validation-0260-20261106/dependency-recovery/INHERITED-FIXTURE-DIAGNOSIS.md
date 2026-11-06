# Inherited fixture diagnosis (read-only)

Both failures were already measured on exact baseline c295af1554f9e80a29864cd013995eccad66cd2c and candidate0260. This continuation made no fixture or production changes and did not rerun their known failing checks. Minimal test-only fixes are appropriate; results remain FAIL until implemented and validated.

## smc-fx-peer-view

The isolated C fixture supplies blit_hash at tests/host/smc_fx_peer_view_test.c:25 but omits popup_knob_full_hash. Production defines it at firmware/src/app/ui.c:4510; extracted smc_ui_off/smc_ui_begin write it at firmware/src/app/smc_mixer.c:785,827. The production extractor at tests/host/smc_fx_peer_view_test.py:47-49 correctly preserves these bodies, so compilation fails on the missing fixture-owned variable.

Add `static uint32_t popup_knob_full_hash;` beside blit_hash in the test fixture. Initialize a nonzero sentinel before smc_ui_begin and assert it clears, then reseed and assert it clears on timeout. Keep all peer/context/timeout/held-state assertions, production-body extraction, and -Werror unchanged. No production edit is needed.

Affected command after repair (NOT RUN in this continuation): `python3 tests/host/smc_fx_peer_view_test.py`.

## smc-ui-negative

The cryptic-footer-returned mutant at tests/host/smc_ui_negative_test.py:35-38 targets removed `cv_commit(3, 0, KNOB_Y)`. Production now uses unique `cv_commit_popup_knobs();` at firmware/src/app/smc_mixer.c:1114. It clears the bottom canvas at1070, draws meaningful status at local y49 at1113, then commits. No obsolete symbol footer is present.

Change only that mutant anchor/replacement to:

```python
"cryptic-footer-returned": (
    '    cv_commit_popup_knobs();',
    '    cv_text(5, 69, &FONT_T, "> R << >> [< >] ^ v < >", C_GRAY);\n    cv_commit_popup_knobs();',
),
```

The restored glyph at local y69 maps to screen227 (KNOB_Y158), inside the existing black-footer check at tests/host/smc_ui_test.c:662-665 (rows223-239). The mutant should therefore fail CHECK(!footer_pixels). Preserve unique-anchor checking and runtime-assertion/zero-exit rejection at negative_test.py:74-85. All other13 anchors match exactly once. Do not count compile failure as a killed behavioral mutant.

Affected command after repair (NOT RUN in this continuation): `python3 tests/host/smc_ui_negative_test.py`.

Extra legacy smc_peer_names fixture remains a separate historical failure (duplicate plat_ble_name and obsolete bt_peer_text); no scope expansion or fix applied.
