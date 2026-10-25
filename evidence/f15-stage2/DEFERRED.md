# Deferred from stage 2 (F15 BF-034 only)

Not implemented in this repair:

| Item | Why deferred |
| --- | --- |
| Withheld asset cache | Not a tip FAIL; excluded by audit. |
| Unmerged data-safety physical-endpoint / identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` in installer HTML | Existing initial-loader resume path; design, not tip FAIL. |
| Lengthening `SLOT_PROBE` retries / timeout | Optional hardening; still deferred — do not lengthen in this cycle. |
| Delete Felucca-era `make_site.py` entirely | Decision still open; not a tip FAIL. |
| Nightly-repo schema second qualification field | Not required for BF-034. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; not a tip FAIL. |
| BF-001–BF-013 / FP-001 | Already in tip ancestry; not reopened. |
| Arp/Fx sole-owner leftovers (`bc-8efb2f94…`) | Explicitly not stolen. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
| Firmware `nightly` fast-forward | Stage 5 only. |
| LX-73 / package republish | Stage 7 / unrelated; Stage 2 does not republish. |
