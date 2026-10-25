# Deferred from stage 2 (F09 BF-005 only)

Not implemented in this repair:

| Item | Why deferred |
| --- | --- |
| Withheld asset cache | Not in tip as an open source defect; excluded by audit. |
| Unmerged data-safety physical-endpoint / identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` in `web/x0x_installer.html` and `web/index_pkg.html` | Left as the existing initial loader resume path (F02 design). |
| Lengthening `SLOT_PROBE` retries / timeout | Optional hardening only; still 1.0s/2 native and 1000ms/2 web after F03 fail-closed. |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; callers still import helpers — not a tip FAIL. |
| Nightly-repo schema extension for a second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; current F08 Stage-7 CHECKS already dual-labels; not a tip FAIL. |
| BF-001 | Already fixed in tip (`d11b74fc`); not reopened. |
| BF-002 | Already fixed in tip (`ab90f47c`); not reopened. |
| BF-003 | Already fixed in tip (`1325f5d3`); not reopened. |
| FP-001 | Already fixed in tip (`6df0f81f`); not reopened. |
| BF-004 | Already fixed in tip (`ce690772`); not reopened. |
| BF-006 drum808 `d8_exp2_cr` signed shift UB | Younger than BF-005; not selected this cycle. |
| BF-007 legacy BD PAN motion migration order | Younger; not selected. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
| Firmware `nightly` fast-forward | Stage 5 only; not this thread. |
