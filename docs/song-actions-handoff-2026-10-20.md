# Song action and confirmation repair

Receiving owner: firmware-only UX coordinator `01a158db-b402-73c3-acdc-e13eaa53d71c`.

Base: `79f251a5b2797941718ad5fa703243f74b6a6640`.
Lane: `lane/sb-ux-song-actions-20261020`.
Worktree: `/Users/mac/Documents/x0x37/ux_song_actions_20261020`.
Commit: use the checked lane tip. The completion response gives its exact hash.

## Repair

- Full-song insertion now reports `SONG FULL`. It changes no song bytes and does not mark the project dirty.
- INSERT BAR and DELETE BAR on the blank append row now report `NO BAR SELECTED`. They preserve existing no-op semantics and dirty state.
- Normal insertion and deletion semantics are unchanged.
- A cancelled confirmation restores the overlay that opened it. Song and Global lists retain selection and scroll position.
- Confirmation still runs the selected action and keeps its prior completion behavior.
- SMC R7 cancellation reaches the same `button_tap(B_HOME)` path. No SMC mapping or source changed.

Changed files:

- `firmware/src/app/ui.c`
- `tests/host/song_actions_test.c`
- `tests/host/song_actions_test.sh`
- `BEHAVIOR.md`
- `docs/song-actions-handoff-2026-10-20.md`

## Checks

All Linux checks used local image `x0x37-validation:local`, image ID `sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1`.

- `CC=cc CXX=c++ sh host/build_host.sh`: PASS. Result: `host: build/host/x0x_host`.
- `CC=cc tests/host/song_actions_test.sh`: PASS. Result: `song_actions: ok`.
- `CC=cc sh tests/host/copy_semantics_test.sh`: PASS. Result: `copy_semantics: ok`.
- `CC=cc sh tests/host/run_project_save_preflight.sh`: PASS. Result: `memory-full preflight: no writes; prior project survives reboot`.
- `MODEL=pro ./build.sh --board smk37`: PASS. App 549736 B of 552672 B; 2936 B free. Static RAM 97764 B of 98304 B. Pool 335700 B of 344064 B. Package 651392 B, identity `SMK-37 Pro_900`.
- `MODEL=elite ./build.sh --board smk37`: PASS. App 549752 B of 552672 B; 2920 B free. Static RAM 97764 B of 98304 B. Pool 335700 B of 344064 B. Package 651396 B, identity `SMK-37 Elite_900`.
- Both target builds passed locked-setting, `.ram_text`, flash, RAM, pool, no-soft-double and register-access checks.
- `git diff --check`: PASS before commit.

The first macOS test attempt did not compile. Strict C99 hid BSD typedefs used by `libproc`, and generated headers were absent. The supported Linux run above generated the headers and passed. No product defect was inferred from that host setup failure.

No FM-1 target build ran. FM-1 is reference-only. No formal gate, external push, merge, install or device write ran. Device behavior is **NOT TESTED**. Build output is local evidence only.

## Integration

Cherry-pick the lane commit onto the UX integration base. Another lane owns `draw_main` readout rendering in `ui.c`; resolve overlap without taking its rendering changes. Rerun the combined affected tests and Pro/Elite builds. Run the applicable formal gate before any separately authorized install.
