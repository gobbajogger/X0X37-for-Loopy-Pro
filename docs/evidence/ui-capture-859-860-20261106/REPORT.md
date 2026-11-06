# Candidate-pinned UI capture evidence

Actual production-host rendering and interaction capture completed on both unresolved UI candidates. This is a bounded evidence contribution for the existing UI owner and reviewer, not a new source owner, formal review round or candidate selection.

## Identities

- PR859: `eea971c05ac35e91f7ba9449638221d985446f13`, tree `1a0c828e174db2ebba3534474cd82ec3d6a3219c`.
- PR860: `c97587c8cea40a8f6e534ec13d9690feb555486d`, tree `ccf31b6fa52217f79350041f3db8643e12b81166`.
- Both target `wip/one-build@48e92b76c2f476cf40771e00a04e1838136bb55d`.

Authenticated GitHub PR reads and Git-fetched objects agree. Each detached worktree has zero tracked changes after capture. Only separate evidence files and ignored generated build files were created. The evidence manifest records commit/tree, generated-input hashes, harness hash, compiler, commands and frame hashes.

## Executed capture

GCC 14.2.0; existing `tests/host/smc_ui_test.sh` builds production ui.c, SMC input dispatch, project and engine sources. The external `capture.c` includes the existing fixture with its original main renamed; it adds no firmware changes or substituted renderer. `ui_frame()` renders into the existing 240x240 RGB565 simulated LCD, and the existing `png_write()` exports each PNG. Input is simulated panel button/SMC MIDI dispatch, not a physical controller. `setup()` and platform callbacks are host fixtures.

Commands, run from the corresponding detached source root:

```sh
CC=gcc SAN_FLAGS='-I/workspace/ui-evidence-859/tests/host' SMC_UI_TEST_FILE=/workspace/ui-render-evidence/capture.c SMC_UI_OUT=/workspace/ui-render-evidence/859 sh tests/host/smc_ui_test.sh
CC=gcc SAN_FLAGS='-I/workspace/ui-evidence-860/tests/host' SMC_UI_TEST_FILE=/workspace/ui-render-evidence/capture.c SMC_UI_OUT=/workspace/ui-render-evidence/860 sh tests/host/smc_ui_test.sh
```

Both commands exited **0**, each with **115 capture-specific assertions** and **12 PNGs**. Compiler warnings are retained in the original local logs; the bundle includes the complete capture stdout rather than compiler source excerpts. The manifest hashes the original logs. Required original-input check on PR860 exited 0 (`10 active`, manifest `9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc`). No private source, generated headers, native binaries or host executables are included in the evidence bundle.

Observed cases:

1. Dark and light saved/queued badges: current steps {0,8}, queued {2}; fixed expected framebuffer columns pass, and rendering preserves the full project.
2. Panel PLAY and simulated SMC PLAY cancel CLEAR PATTERN, leave the project unchanged, and request one transport start. Held SMC repeat retains that request. This is request-state evidence, not real audio or running-device transport proof.
3. Simulated SMC HOME+SELECT opens RESET SOUND 909. Cancel preserves the project. Confirmation changes only the selected saved 909 sound; mixer settings remain unchanged. Undo restores the complete before-project and redo restores the complete expected after-project.
4. The physical HOME event navigates to SONG before reset confirmation in both candidates; the trace and captures expose this behavior. No new judgment about that existing routing policy is made.

The 12 corresponding image pairs are pixel-identical: **0 differing pixels out of 57,600 per pair**. This establishes equality only for the captured states, not source equivalence or general candidate interchangeability. The source candidates differ in 12 paths, including production ui.c, smc_mixer.c and sound_reset.c.

The contact sheet was visually inspected: badge, question, cancel and reset/undo/redo frames render and match the trace. This inspection is not the required historical independent design approval.

## Existing handoff and remaining limits

- Existing QA requirement: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/860#issuecomment-6674866179
- Existing integrator accounting: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/698#issuecomment-6676551791
- PR859: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/859
- PR860: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/860

Fresh PR discussion reads found no newer candidate selection or producer reply linking approved presentation evidence. The PR860 source plan describes implementation and source review but does not pin an approved before/after visual presentation. The fixture's historical #737 reference concerns SMC peek policy and its PR describes a proposed peek mockup; it does not establish approval for these candidate-specific saved/queued badges or reset controls. Older engines-UI plan references are likewise not candidate-specific visual approval. This bounded search does not prove such evidence is absent elsewhere.

This delivery supplies new actual host-render and interaction evidence for the existing reviewer to consume. Remaining requirements: existing owner/integrator selects or dispositions the competing candidates, links the applicable approved presentation and independent design-review record (or explicitly resolves the missing stage), and evaluates these captures against that reference. No claim that the complete visual-stage requirement is closed.

Unchanged full suites were not repeated. Native Elite/Pro, controller packaging, real-device interaction, publication and combined-tree qualification were NOT_RUN. No shared branch/ref writes, commits, PR comments, merge, publication, ownership transfer or device I/O occurred.
