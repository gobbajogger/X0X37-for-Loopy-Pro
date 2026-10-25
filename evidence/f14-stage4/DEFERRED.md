# Deferred from stage 3 (F14 BF-013 only)

Carried forward unchanged from `evidence/f14-stage2/DEFERRED.md`. Stage 3 did not implement any deferred item.

| Item | Why deferred |
| --- | --- |
| Withheld asset cache | Not in tip as an open source defect; excluded by audit. |
| Unmerged data-safety physical-endpoint / identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` in `web/x0x_installer.html` and `web/index_pkg.html` | Left as the existing initial loader resume path (F02 design). |
| Lengthening `SLOT_PROBE` retries / timeout | Optional hardening only; still 1.0s/2 native and 1000ms/2 web after F03 fail-closed. |
| Delete Felucca-era `make_site.py` entirely | Decision still open after BF-003 installer-only trim; callers still import helpers — not a tip FAIL. |
| Nightly-repo schema extension for a second qualification field | Not required: FP-001 dual-digest + host check. |
| Historical F06 Stage-7 “Installer FP” wording rewrite | Docs debt; current Stage-7 CHECKS already dual-labels; not a tip FAIL. |
| PR #74 / `origin/nightly` unrelated history beyond the two BF-013 paths | Only `firmware/src/dsp/fxbus.c` + `tests/host/stereo_test.c` were in scope. |
| Younger BF-014+ already on diverged `origin/nightly` | Not selected ahead of BF-013 on the published tip; out of this Stage 3. |
| BF-001–BF-008 / BF-012 / FP-001 | Already fixed in tip; not reopened. |
| Arp/Fx leftovers from #279/#281 | Sole-owner `bc-8efb2f94…`; not stolen. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
| Firmware `nightly` fast-forward | Stage 5 only; not this thread. |
| Elite / Pro development packages | Stage 4 only; host PASS is not a package PASS. |
