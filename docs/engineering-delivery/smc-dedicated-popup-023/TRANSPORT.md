# Transport popup correction

Owner rule: PLAY (`0x5E`) and MMC Play/Stop perform transport actions without
opening a popup. Rewind/Forward (`0x5B`/`0x5C`) show the existing kind-3 action
popup with the cued pattern. Returning to the playing pattern cancels its cue and
shows the current pattern. No new screen, renderer or persistent state.

Changed from `02c5224d12c7a96c0061fea71f13de04de58741b` on the same PR #789.
The regression tests use five production queue paths: BLE DAW notes, BLE User
MMC, host User notes, USB MMC and peripheral BLE MMC. They cover transport
requests without popup activation, locked and unlocked pattern targets, repeated
cues, cue cancellation, lower boundary, expiry and keyboard-state invariance.
The pattern number and status are checked against the actual framebuffer.

Both new cases fail on the starting head at the intended assertions: PLAY opens
a popup, and Forward has no kind-3 popup. The expanded popup suite passes 4183
checks after the fix (465 additional checks). The existing registered test owns
these regressions; no new runner or test script was added.

[Actual pattern-cue framebuffer](framebuffer/after-pattern-cue.png): 240x240,
`909 PATTERN / P5 / CUED`, captured by the production `smc_ui_draw_peer` path.
PNG compression preserves every framebuffer pixel.

Elite and Pro native builds PASS with SDK V1.2.1 and `X0X_DEV_PADCAL=0`.
Both use 95872/98304 B static RAM: **2432 B free**, unchanged from the starting
head and 80 B above base 0.0.23. Pool free is 8536 B. App sizes: Elite 497868 B,
Pro 497852 B. The first overlapping build picked up the host break-bank header
and overflowed flash; an isolated native worktree removes that generator race.
The final builds use the same reviewed firmware diff and verified SDK/toolchain
pins. Firmware binaries are excluded.

Compound Engineering reviewed correctness, project standards, tests and the
adversarial lens: no actionable findings. Simplification was skipped to preserve
the requested minimal fix. Optional second-peer pattern-footer coverage remains
outside this five-route peer-zero regression matrix.

Production diff: +10/-3 lines in `smc_mixer.c`; tests: +90 lines in the existing
popup test. Protected source/settings paths and keyboard navigation writes are
unchanged. Source SHA-256:
`8edbb7fc6e1b6233898b93542ff11f020b360c7ea511d49173941a87ef054ae2`.

Fresh scoped run: **13 PASS / 0 FAIL / 0 selected NOT_RUN**:
`smc-buttons`, `smc-mixer-edges`, `smc-popup-engine`, `smc-audit-core`,
`smc-audit-contract`, `smc-mode-page`, `smc-mixer-page`, `smc-ui-bounds`,
`smc-ui-negative` (all fourteen production fault checks),
`smc-performance-abort`, `smc-filter`, `smc-feedback`, `smc-record-feedback`.
The runner's unselected package-test footer is separate from these selectors;
Elite/Pro native packages were built independently as described above.
The expanded popup test separately passes all 4183 checks.
The earlier audit belonged to the starting head;
its nine failures are now resolved; see the fresh 47-selector run in TESTS.md. The full host suite and hardware checks remain
NOT_RUN. PR #789 remains a draft.
