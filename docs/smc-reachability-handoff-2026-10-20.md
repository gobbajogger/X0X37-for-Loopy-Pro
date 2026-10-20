# SMC reachability repair

Receiving owner: the assigned firmware integration chat. No message, external push, install or device write was performed.

Base: `53ed908c92e3a112a5d5c1ec8d1dddf303bf57e8`, the recorded installed revision. The integration checkout remained clean at this revision when rechecked. Repair lane: `lane/mb-smc-reachability`, in `/Users/mac/Documents/x0x37/smc_reachability`. Resolve the checked repair commit with `git rev-parse lane/mb-smc-reachability`; the completion reply supplies its exact hash.

## Findings and repair

The `91356e7` remap removed next-page encoder assignments and made faders use the main mixer. This changed direct access, but ordinary pages still had page arrows. The missing menu entry and individual drum selection were older gaps. Misleading pickup feedback also predates the remap. The owner has not yet identified a specific unreachable parameter or confirmed whether the current feedback is X0X notices or SMC lights. No current transport failure is established.

The repair keeps main-mixer faders and master-filter fader 8, visible-field encoder assignments and drum banking. It adds view navigation, part sound entry, deep-menu open/activate/back, explicit confirmation, and drum selection using stock buttons. Clip pages have stage and field navigation, including ARP LATCH. Clip-grid encoders no longer edit the hidden X0X page. The [mapping guide](SMC_MIXER.md) defines the shortcuts.

The real project defines `NDRUM` as **11**. Both engines have eleven drum voices followed by kit-wide controls. The old test declared twelve independently of production, so its claimed twelve-track PASS was invalid. This repair exposes all eleven actual tracks and keeps both banks; bank two uses three faders. No drum was removed, no twelfth drum was invented, and no project format was changed. Historical handoffs remain unchanged; this finding supersedes their twelve-track claim.

Waiting faders show `PICK F…`; acquired faders show `SMC F…`. Target/value changes clear both pickup and crossing history. The real `smc_notice`, `smc_frame`, `pref_set`, `motion_touch` and display-message path are exercised together. Waiting does not allocate motion; pickup crossing does. A following frame retains pickup after an SMC write. External changes and reconnects reset it. Help and questions block edits; R6 confirms and R7 cancels. Input is drained during performance tests, including after a menu action starts one within a queue batch.

## Validation

Commands ran in the repair lane. Logs are in [evidence/smc-repair](../evidence/smc-repair/).

- `X0X_HOST_DEFS=-D_DARWIN_C_SOURCE X0X_TEST_JOBS=1 bash tests/run_tests.sh smc-protocol smc-fader smc-buttons smc-mapping smc-filter smc-part-select smc-clip-visibility smc-reachability`: PASS. See `tests.log`. The test runner's package-test skip is outside this selected host-test scope.
- The new end-to-end harness includes production project, UI and engine sources, all SMC routing/queues, and extracted production clip navigation and encoder handlers. It runs complete sequences through BLE DAW, BLE User MMC and host User MMC/CC. Hardware I/O and clip field storage are substitutes. This is not a hardware transport or audio validation.
- Both kits: every actual track, sound-page selection, mix pan/reverb/delay edits, bank bounds, and inactive second-bank faders/SELECT buttons. All part sound pages and deep-menu rows, all ordinary views, Global, clip editors, confirmation/help/performance guards and pickup/motion/reset paths pass.
- The same harness compiled against the base's `smc_mixer.c`, `main_fm1.c` and `smc_protocol.h` fails at SELECT not opening sound pages (`base-navigation.log`). With `SMC_PICKUP_ONLY=1`, it fails at the missing waiting-pickup notice (`base-pickup.log`). These reproduce older gaps on the installed baseline; they do not date those gaps to the latest remap.
- Elite and Pro, release (`X0X_DEV_PADCAL=0`) and development (`=1`): `MODEL=<model> bash build.sh --board smk37`, using the local Linux x86-64 Docker toolchain. Each build performs locked-setting, flash, RAM, pool, no-soft-double and register-access checks. See `build-<model>-<0|1>.log` for exact sizes.
- `git diff --check` and docs-only quickcheck: PASS. Grammar hints are advisory.

Final sizes are identical between Pro and Elite for the app:

| Configuration | App bytes / limit | Static RAM / limit | Pool / limit |
| --- | --- | --- | --- |
| Release | 547704 / 552672 | 97348 / 98304 | 335700 / 344064 |
| Development | 549704 / 552672 | 97764 / 98304 | 335700 / 344064 |

No features, sample quality, memory limits, model guards or controller sources were reduced or changed. No formal gate was run. No firmware was installed. Device behavior is **NOT TESTED**.

## Integration and device check

Cherry-pick the lane commit onto the integration owner's chosen current base. Review conflicts in `smc_mixer.c` and the clip helpers in `main_fm1.c`. Rerun affected checks on that combined source and the applicable install/release gate before any separately authorized installation.

Build: this local repair, not an installed image. Replaces: the SMC navigation and pickup behavior of installed `53ed908`. Checked against decisions: preserve main-mixer/master-filter mapping and banking; no feature or sample-quality cuts; integration owner controls firmware installation.

After a separately authorized install, check these device tasks:

1. Starting at HOME, SELECT a part, open its sound pages and deep menu, and edit a previously unreachable parameter.
2. Starting on each drum MIX page, select all eleven tracks across both banks and edit pan and sends.
3. Starting with a physical fader away from its target, observe waiting feedback, cross the target and verify the actual value changes.
4. Starting in clip mode, open SCALE/CHORD/ARP, edit ARP LATCH, return to the grid and confirm hidden sound parameters do not change.
5. Starting at a menu action that asks a question, verify blocked edits and explicit R6/R7 confirm/cancel. Check notes, pad colours and Bluetooth remain working.
