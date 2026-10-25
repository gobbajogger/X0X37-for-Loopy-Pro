# Main-loop SMC input pointer

Reason: poll and frame were still computing the address of a 240-byte per-peer input from the peer index. The application image remains over its flash budget after the selected-state pointer.

Purpose: keep one main-loop pointer to the selected direct peer's input, updated in the existing selector. TIMER5 still indexes `smc_inputs` by the peer argument it was given. Selecting the hosted slot does not move the input pointer, because that slot has no input object.

Effect for the person playing: none intended. Targets, locks, pickup, Pause, Solo and two-mixer isolation stay as they were. This change does not make the package fit, and it is not hardware-supported firmware.

## Rejected experiment

Outlining one shared Record/Solo/Mute/SELECT press for part strips and drum strips matched 378 transcripts against the handoff input, and host `-Oz` text grew by 201 bytes (`smc_button_apply` 1828 to 1310 plus `smc_role_press` 719). That version was not kept.

## Kept change

`smc_select` sets `smc_input_current` for a direct peer. Queue, link and release reads in poll and frame use that pointer.

Host `-Oz` text of the production peer harness, supporting evidence only: 10154 bytes to 10118 bytes, delta -36. `sizeof(smc_ble_input_t)` on that host was 240. Native Elite and Pro sizes are not measured here. A plausible native result is a similar small drop in `smc_poll` and `smc_frame`, not the remaining 3516-byte application overflow and not the controller overflow.

## Checks

Differential against `61a38c274dbdf7aa98c9714e26263030952fea8d` matched the full 256-cursor peer script, including the end state of both inputs.

GCC ASan and UBSan:

- `smc_lock`: PASS, 674 checks
- `smc_pause_loss`: PASS, 397 checks
- `smc_navigation`: PASS, 165 checks
- `smc_multi_peer_application`: PASS, 507 checks
- `smc_ui`: PASS, 5893 checks
- `smc_multi_peer_test.py`: PASS, all 256 cursors

Clang ASan could not link: `libclang_rt.asan-x86_64.a` is not installed. Pickup acceptance was not re-run; its historical exit 139 remains failed and inconclusive.

Independent review of this commit is still open. This author is not that reviewer.
