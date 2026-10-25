# Deferred from stage 2 (F14 BF-013 only)

Not implemented in this repair:

| Item | Why deferred |
| --- | --- |
| Withheld asset cache | Not in tip as an open source defect; excluded by audit. |
| Unmerged data-safety physical-endpoint / identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` in installer loaders | Left as the existing initial loader resume path (F02 design). |
| Lengthening `SLOT_PROBE` retries / timeout | Optional hardening only; not a tip FAIL. |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; not a tip FAIL. |
| Nightly-repo schema extension for a second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; not a tip FAIL. |
| Absorb diverged `origin/nightly` pot-curve / XIP / BF-014+ history | Audit tip repair only; Stage 5 integrates `nightly` when packages allow. |
| BF-001–BF-008 / BF-012 / FP-001 | Already fixed in audited tip; not reopened. |
| Arp/Fx leftovers from #279/#281 | Sole-owner `bc-8efb2f94…`; not stolen. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
| Firmware `nightly` fast-forward | Stage 5 only; not this thread. |
