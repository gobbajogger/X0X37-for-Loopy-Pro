# Deferred from stage 2 (F07 FP-001 only)

Not implemented in this repair:

| Item | Why deferred |
| --- | --- |
| Withheld asset cache | Not in tip as an open source defect; excluded by audit. |
| Unmerged data-safety physical-endpoint / identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` in `web/x0x_installer.html` and `web/index_pkg.html` | Left as the existing initial loader resume path (F02 design). |
| Lengthening `SLOT_PROBE` retries / timeout | Optional hardening only; still 1.0s/2 native and 1000ms/2 web after F03 fail-closed. |
| Delete Felucca-era `make_site.py` entirely | Younger / decision still open after BF-003 installer-only trim; not selected over FP-001. |
| Nightly-repo schema extension for a second qualification field | Not required: `installer_sha256` keeps source-fingerprint semantics; firmware evidence records both digests; host check enforces the split. |
| Rewrite historical F06 Stage-7 CHECKS “Installer FP” wording | Left for F07 Stage-7 republish evidence after Stages 3–5; tooling + `digests.json` pattern land here. |
| BF-001 | Already fixed in tip (`d11b74fc`); not reopened. |
| BF-002 | Already fixed in tip (`ab90f47c`); not reopened. |
| BF-003 | Already fixed in tip (`1325f5d3`); not reopened. |
| Firmware / controller / package bytes | Unchanged by design. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
