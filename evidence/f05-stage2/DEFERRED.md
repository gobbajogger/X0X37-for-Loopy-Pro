# Deferred from stage 2 (F05 BF-002 only)

Not implemented in this repair:

| Item | Why deferred |
| --- | --- |
| BF-003 | Younger. Stale `web/test_web.mjs` → missing `editor.html`. Needs a retire-versus-restore decision before any Stage-2 change. Decision still open — leave deferred. |
| Withheld asset cache | Not in tip as an open source defect; excluded by audit. |
| Unmerged data-safety physical-endpoint / identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` in `web/x0x_installer.html` and `web/index_pkg.html` | Left as the existing initial loader resume path (F02 design). |
| Lengthening `SLOT_PROBE` retries / timeout | Optional hardening only; still 1.0s/2 native and 1000ms/2 web after F03 fail-closed. |
| Stage-7 installer fingerprint pin mismatch vs served HTML | Evidence-pin discrepancy only; not a package FAIL and not part of BF-002. |
| BF-001 | Already fixed in tip (`d11b74fc`); not reopened. |
| Firmware / controller / package bytes | Unchanged by design. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
