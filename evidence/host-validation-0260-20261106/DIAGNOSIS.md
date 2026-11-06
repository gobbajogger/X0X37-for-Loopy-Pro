# Read-only diagnosis and minimal repair scope

Three parallel agents inspected existing candidate/baseline source and logs. No tests were rerun, no source was changed, and no alternate compiler environment was used. All 21 candidate-only failures share obsolete flat-GLOBAL navigation or source-shape assumptions. This is a fixture integration defect with downstream functionality still unproven, not proof of 21 separate production defects.

Candidate `firmware/src/app/ui.c:1549-1558,1599-1601` opens R_SECTION category rows. `global_section` at 1560-1589 holds the former settings/actions: MIDI+ CLOCK channels at 1565-1567; PROJECT actions at 1580-1582; SYSTEM/performance/pad calibration at 1583-1586. Section entry is at 3623; `<` backs out at 3658-3663; HOME leaves a section before closing GLOBAL at 3787. Existing recorded ui-settings-sections PASS covers 494 assertions, but does not replace failed downstream regressions.

## C and source-shape fixtures (five baseline PASS)

| Selector | Location | Minimal repair |
|---|---|---|
| project-runtime | tests/host/project_runtime_test.c:38-43 | Enter PROJECT through its R_SECTION row before searching LOAD; retain runtime/storage assertions. Failure also measured on v6, direct exit 134. |
| part-order | tests/host/part_order_test.c:69-71 | Enter MIDI + CLOCK before counting R_MIDICH. Keep NPARTS and part_at ordering expectations. |
| song-actions | tests/host/song_actions_test.c:101-104 | Enter PROJECT before locating CLEAR PATTERN. Missing row -1 currently cascades into invalid selection/cancellation failure. Keep confirmation and state-preservation assertions. |
| smk-padcal-wiring | tests/smk_padcal_wiring_test.py:30-32 | Stop requiring GR_SYSTEM/GR_BT markers inside open_global. Inspect global_section(5) or navigate behaviorally; keep SMK/dev/not-clip guards. |
| smk-arrows-owner | tests/smk_arrows_owner_test.py:36 | Accept/check the guarded ui_menu_arrow(d) call and arw_rearm at firmware/src/app/main_fm1.c:1099-1101. Preserve routing/masking checks. |

## MIDI/clock/arrow scenarios (eight baseline PASS)

| Selector | Stale fixture location | Minimal repair |
|---|---|---|
| scenario-arrows | tests/scenarios/arrows.x0x:23-31 | Add category entry/back assertions and local row indices. `<` now navigates back; keep value-decrement coverage with ALGO. |
| scenario-clip-arrow-menu | tests/scenarios/clip-arrow-menu.x0x:12-23 | Enter MIDI section; adapt held-arrow/back semantics while preserving clip/menu ownership assertions. |
| scenario-clklock | tests/scenarios/clklock.x0x:1040-1042 | Enter MIDI after GLO before changing clock source; preserve clock assertions. |
| scenario-clkmove | tests/scenarios/clkmove.x0x:8-16 | Enter MIDI before ALGO edits; account for section exit. |
| scenario-clocksrc | tests/scenarios/clocksrc.x0x:4-7 | Enter MIDI before ALGO edits; preserve external-clock/transport assertions. |
| scenario-tour | tests/scenarios/tour.x0x:95-106 | Enter PROJECT and RESET row 4 instead of applying old global offset. |
| scenario-probcond | tests/scenarios/probcond.x0x:30-39 | Enter MIDI, then enable NOTES OUT at local row 2; retain expected noteons 22. |
| scenario-probreset | tests/scenarios/probreset.x0x:14-26 | Enter MIDI, then enable NOTES OUT at local row 2; retain expected noteons 3. |

The probability fixtures never enable NOTES OUT, explaining noteons=0 without demonstrating a probability defect. clkmove/clocksrc never change settings, so no expected stored object exists: host/x0x_host.c:811-812 only recognizes storesetN inside store_len[OBJ_SET]. Those unknown-expect errors are cascading setup symptoms.

## Storage/performance scenarios (eight baseline PASS)

| Selector | Stale fixture location | Minimal repair |
|---|---|---|
| scenario-autosave | tests/scenarios/autosave.x0x:33 | Enter PROJECT, select AUTOSAVE local row 2; preserve storage counts. |
| scenario-guard | tests/scenarios/guard.x0x:49 | Enter SYSTEM then PERFORMANCE row 0; preserve overload/guard assertions. |
| scenario-migrate | tests/scenarios/migrate.x0x:31 | Enter PROJECT, RESET row 4, retain confirmation and revision rejection/save assertions. |
| scenario-migrate0102 | tests/scenarios/migrate0102.x0x:31 | Same navigation repair; do not restore forbidden migration compatibility. |
| scenario-perf | tests/scenarios/perf.x0x:14 | Enter SYSTEM then PERFORMANCE; preserve results/transport assertions. |
| scenario-perfsave | tests/scenarios/perfsave.x0x:11 | Enter SYSTEM then PERFORMANCE; preserve storage/performance assertions. |
| scenario-undo | tests/scenarios/undo.x0x:69 | Enter PROJECT then RESET; retain undo/live-state assertions. |
| perf-scratch | tests/run_tests.sh:663, first perfsave invocation | Repair the shared perfsave fixture. Current failure occurs before scratch variant compilation or WAV comparison. |

In default non-SMK scenario host, BLUETOOTH is absent, so PROJECT/SYSTEM category indices are 3/4; SMK includes it, making them 4/5. Prefer semantic navigation instead of new fragile global offsets. Migration fixtures pass their unsupported-store checks before failing navigation; RESET never runs, causing later save failures. The logs do not establish a migration, storage or scratch/DSP defect.

## Affected-only commands after separately authorized fixture fixes

These commands are proposed, NOT RUN in diagnosis. Preserve expected functional values. No broad passing-suite rerun is requested merely for changed metadata.

```sh
sh tests/run_tests.sh project-runtime part-order song-actions smk-padcal-wiring smk-arrows-owner ui-settings-sections
sh tests/run_tests.sh scenario-arrows scenario-clip-arrow-menu scenario-clklock scenario-clkmove scenario-clocksrc scenario-tour scenario-probcond scenario-probreset
sh tests/run_tests.sh scenario-autosave scenario-guard scenario-migrate scenario-migrate0102 scenario-perf scenario-perfsave scenario-undo perf-scratch
```

Two inherited fixture failures (smc-fx-peer-view and smc-ui-negative) and missing Clang remain separate. Full host gate stays FAIL until applicable failures and missing proof receive a valid disposition. No release claim.
