# BF-007: loading legacy project loses 909/808 Bass Drum PAN motion mapping

**2026-10-23 | Severity MEDIUM | F11 Stage 2: FIXED on tip (host regression green)**

On audited source `af494784` / baseline main `c9718ae8…`, `firmware/src/app/project.c` (Git blob `1000a0d88e04553295b82d724a63670434b67748`), `sound_bd_sends()` migrated BOTH BD PAN sound values and the matching motion lane index; `read_project()` invoked it immediately after OBJ_SOUND. Legacy motion lanes and compressed arrangement load only afterward. The lane migration saw defaults. Old projects retained motion index 8 for 909 (expected 10) and 7 for 808 (expected 9), now corresponding to send controls. Static PAN values migrated successfully.

## Repair (this tip)

- Split static sound-value migration (`sound_bd_sends`) from lane migration (`sound_bd_sends_lanes`).
- Keep static PAN migration at the post-OBJ_SOUND site.
- Call `sound_bd_sends_lanes` only after stream/arrange load (and before `snd_rev` is bumped).
- Host regression: `tests/host/bugfinder_legacy_bdpan_motion_test.c` via `tests/host/run_bugfinder_legacy_bdpan_motion.sh`, wired as `legacy-bdpan-motion` in `tests/run_tests.sh`.
- Unfixed tip: FAIL exit 1, 171 checks / 3 FAIL. After repair: PASS 171/171.

## Reproduce

```
sh tests/host/run_bugfinder_legacy_bdpan_motion.sh
# or: X0X_TESTS="legacy-bdpan-motion project-stage" sh tests/run_tests.sh
```

## Ownership / safety

Do not fold open PR #28 `fastmath.h` into this change. BF-001–BF-006 and FP-001 remain closed. No device flash, production Pages, or firmware `nightly` force-push in Stage 2.
