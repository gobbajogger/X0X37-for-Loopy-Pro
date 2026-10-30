# Box host setup for x0x37 host tests (commit cb5423c4)

## Before
- Run: tests/run_tests.sh on cb5423c4, box gcc 14.2 as cc. The run was stopped before the end.
- 53 tests had a .fail file when the run stopped.

## Causes found in the logs
1. Stricter compiler. GCC 14 with -Werror stops on "inline function declared weak" (fastmath.h FM_ONE) and on "'minsize' attribute directive ignored" (smk_clip_keys.h). These attributes are for clang. About 30 tests fail for this cause.
2. System-lib clash. glibc hides POSIX functions (unlink, fdopen) when the test uses -std=c99. macOS does not hide them. Example: rec-held-duration.
3. Missing private headers. host-build, gen-nonfont-tables, gen-order and all scenario-* tests need experiments/original_synth/*_private.h and experiments/wobble/miami_decimator_tables_private.h. These files are in .gitignore. They are not on the box. Many later tests need build/gen output (felucca_font.h) from host-build.
4. Pillow imports correctly on the box. I did not find a missing image lib.

## Changes
- apt install clang: FAILED. The Debian mirror gave HTTP 500. Try again later.
- Added bin/cc (and bin/gcc link): a wrapper for /usr/bin/gcc. It adds -Wno-attributes -Wno-error=attributes -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L.
- Added env.sh. It puts bin/ first on PATH, sets CC, the JieLi SDK and toolchain paths, and the schwung reference dirs.
- No repo code is changed.

## After (checked)
- smk_clip_arp_share.c: gcc fails, the wrapper compiles it.
- rec_held_duration_test.c: the unlink error is gone. It now stops on felucca_font.h, which comes from host-build (cause 3).
- A full-suite count after the fix is not available. The parent stopped all suite runs.

## Open items
- Put the approved private headers on the box (for example /workspace/x0x37-refs/private/) and copy them into each worktree.
- Real repo failures (not setup): host-setup (tools/build.py missing in test copy), editor-ota-dispatch (ED_MAINTENANCE_HELD not declared), smk-clip-knob (ui_boot_flash_busy not declared). These need a confirmation on a full run.
