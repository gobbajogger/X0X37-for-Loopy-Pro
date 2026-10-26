# Installer UI accessibility — EN/JA region and control labels (Run 16)

**Source:** `gobbajogger/X0X37-for-Loopy-Pro`, PR #92 `46299fb0f2a38af5fabc1372790e1b0f84e50dc3`, tree `b417526152125c9c2632190b11ccb6ee49126a89`, HTML blob `0f81dd4bf0c5cac624b38209d349f554e8b161b3` (local git hash matched). Stack source-only PR onto #92; not Pages-live.

**Before:** Switching UI to Japanese left nine `aria-label` names English: home, site nav, steps, connection, version, keyboard model, flash progress, install guidance and resources. Japanese screen-reader users heard English names despite translated visible controls.

**After:** Nine `data-aria-t` hooks read the existing EN/JA TEXT maps through `applyLang()`, retaining English fallback initial DOM. No model, CRC, OTA, sample or flashing behavior changed. Focus remains on the language control.

### Actually executed native checks

- `python3 web/test_install_accessible_language.py ORIGINAL /tmp/before`: exit **1** expected before, assertion at 320px Japanese home aria label (English instead of Japanese).
- `python3 web/test_install_accessible_language.py CORRECTED /tmp/after`: exit **0** at **320/390/1280**, nine labels EN→JA→EN, focus, empty picker, invalid .fwsc and unchecked confirmation, no horizontal overflow/browser errors.
- `node web/test_install_journey.mjs CORRECTED`: exit **0**, seven journey states.
- `node web/test_install_connection_retry.mjs CORRECTED`: exit **0**, Web MIDI retry/rescan focus.
- `node web/test_install_local_entry.mjs CORRECTED`: exit **0**, five local-file negatives.
- `node --check` on extracted module script: exit **0**.
- Full repository combined slot/OTA regression suite **NOT_RUN** (exact full checkout unavailable in Linux container); firmware Elite/Pro compile and device tests **NOT_RUN** (browser-only UX). Screenshot evidence is local. Do not mistake browser mock for physical or HTTP verification.

**Next:** Independent QA should re-run Chromium names/focus and inspect stacked source ancestry; sole Safe Merge integrates reviewed source via firmware `nightly` PR, then NIGHTLY website owners mirror/review and publish independently. Public production picker must stay fail-closed and hardware-proven. See `docs/scheduler-handoffs/installer-aria-language-20261023-run16.json`. Preserve archive/stock/backup content.
