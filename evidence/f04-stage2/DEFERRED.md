# Deferred from stage 2 (F04 BF-001 only)

Not implemented in this repair:

| Item | Why deferred |
| --- | --- |
| BF-002 | Younger. `tools/quickcheck_plan.py` `web/` route still omits `projects-web` / `projects-page`. Explicitly out of stage-2 scope. |
| BF-003 | Younger. Stale `web/test_web.mjs` → missing `editor.html`. Needs a retire-versus-restore decision before any Stage-2 change. Decision still open — leave deferred. |
| Withheld asset cache | Not in tip as an open source defect; excluded by audit. |
| Unmerged data-safety physical-endpoint / identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` in `web/x0x_installer.html` and `web/index_pkg.html` | Left as the existing initial loader resume path (F02 design). |
| Lengthening `SLOT_PROBE` retries / timeout | Optional hardening only; still 1.0s/2 native and 1000ms/2 web after F03 fail-closed. |
| Draft PR #31 stack-merge | Depends on #20/#24; not an ancestor of this tip. Reimplemented cleanly instead. |
| Firmware / controller / package bytes | Unchanged by design. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
