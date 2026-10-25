# Deferred items (stage 2 — F02 only)

Not implemented on this branch. Carry forward for later cycles.

| Item | Why deferred |
| --- | --- |
| F03 | Younger than F02; `slot_probe` / `plan_slots` backup skip. Stage-1 audit withheld. |
| BF-001 | Younger open bugfinder finding. |
| BF-002 | Younger open bugfinder finding. |
| BF-003 | Younger open bugfinder finding. |
| Withheld asset cache | Alias-loader stale hit; failed cache not in tip. |
| Unmerged data-safety identity lanes | Explicitly out of scope for this fail-closed port-name repair. |

## HTML loader scans

`web/x0x_installer.html` and `web/index_pkg.html` only `find()` an already-present OTA port to **resume** an interrupted write. That is not a transition this session commanded. Transition waits go through `Updater.install` / `installKeepingSlots`, which now bind. No unbound post-transition `waitFor` remains on those pages. `ambiguous` UI strings were added for the new fail-closed code.
