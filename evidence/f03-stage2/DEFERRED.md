# Deferred from stage 2 (F03 only)

Not implemented in this repair:

| Item | Why deferred |
| --- | --- |
| BF-001 | Named only; no definition in repo. Explicitly out of stage-2 scope. |
| BF-002 | Named only; no definition in repo. Explicitly out of stage-2 scope. |
| BF-003 | Named only; no definition in repo. Explicitly out of stage-2 scope. |
| Withheld asset cache | Not in tip as an open source defect; excluded by audit. |
| Unmerged data-safety physical-endpoint / identity lanes | Explicitly not imported. |
| HTML resume `find(IS_OTA)` in `web/x0x_installer.html` and `web/index_pkg.html` | Left as the existing initial loader resume path (F02 design). |
| Lengthening `SLOT_PROBE` retries / timeout | Optional hardening only; not required for the fail-closed minimal fix. |
| Firmware / controller / package bytes | Unchanged by design. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
