# Deferred from stage 2 (F02 only)

Not implemented in this repair:

| Item | Why deferred |
| --- | --- |
| F03 | Younger than F02; `slot_probe` / `plan_slots` backup skip. Explicitly out of stage-2 scope. |
| BF-001 | Younger open bugfinder finding. |
| BF-002 | Younger open bugfinder finding. |
| BF-003 | Younger open bugfinder finding. |
| Withheld asset cache | Not in tip as an open source defect; excluded by audit. |
| Unmerged data-safety physical-endpoint lanes | Explicitly not imported. F02 uses newly-appeared port snapshots, not `_bound_port` identity binding. |
| HTML resume `find(IS_OTA)` in `web/x0x_installer.html` and `web/index_pkg.html` | Those scans pick up a device already in the loader (interrupted upgrade). They do not continue a transition this install started, so the newly-appeared rule has no pre-transition snapshot. Left as the existing initial loader resume path. Transition waits go through `fm1ota.js` / `installKeepingSlots`. |
| Initial "first running keyboard" scan | Out of scope; not turned into a device picker. |
| Firmware / controller / package bytes | Unchanged by design. |
| Production `main` / production GitHub Pages / Stage 6 flash | Unchanged; cloud does not flash. |
