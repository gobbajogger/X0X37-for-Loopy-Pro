# Deferred from stage 2 (F06 BF-003 only)

Not implemented in this repair:

| Item | Why deferred |
| --- | --- |
| Withheld asset cache | Not in tip as an open source defect; excluded by audit. |
| Unmerged data-safety physical-endpoint / identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` in `web/x0x_installer.html` and `web/index_pkg.html` | Left as the existing initial loader resume path (F02 design). |
| Lengthening `SLOT_PROBE` retries / timeout | Optional hardening only; still 1.0s/2 native and 1000ms/2 web after F03 fail-closed. |
| Stage-7 installer fingerprint pin mismatch vs served HTML | Evidence-pin discrepancy only; not a package FAIL and not part of BF-003. Optional later docs/evidence pass. |
| Delete Felucca-era `make_site.py` entirely | Audit left installer-only trim vs full delete open; this repair only stopped the editor copy. |
| Restore `web/editor.html` / `firmware/src/editor.c` | Explicitly rejected — retire path approved by F06 Stage 1. |
| BF-001 | Already fixed in tip (`d11b74fc`); not reopened. |
| BF-002 | Already fixed in tip (`ab90f47c`); not reopened. |
| Firmware / controller / package bytes | Unchanged by design. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
