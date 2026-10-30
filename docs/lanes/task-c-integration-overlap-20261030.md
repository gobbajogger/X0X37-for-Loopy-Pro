# Task C ownership and integration comparison — 2026-10-30

## Source pins

- Task C isolated draft: [#738](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/738), original FMB handoff `ae0a62e3774545d4dc7544d481b0d9da10b26055`, reviewed checkpoint `1d0978e62e7f1c3030dd96a6abadb331d099b362`.
- Frozen integration: [#734](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/734), `integration/all-in-one@c9fd704641116cc7d6f35602d06d2c84b5cc1fff`.
- Separately owned SMC popup port: [#737](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/737), `lane/mb-smc-peek-734@2f97c31abb72577380b73b621dd4c8f834e248ac`.
- FX mixer UI is owned independently by [#736](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/736), not this branch.

## Exact Git tree findings

Authenticated recursive Git tree reads show these blob identities identical between the FMB handoff and the frozen #734 head:

| Source | Shared blob prefix |
| --- | --- |
| `tests/host/ui_main_material_test.c` (#650) | `90efbe65a4` |
| `firmware/src/app/chicago_ens.h` (#651) | `348823f836` |
| `firmware/src/app/chicago_param_tables.c` (#651) | `500c43ceaf` |
| `firmware/src/dsp/drum909_dsp.h` (#651) | `1fb784dfef` |
| `firmware/src/dsp/fastmath.h` (#651) | `4159a40a52` |
| `firmware/src/ble/ble_midi_central.c` (FMB #687) | `791cf4070b` |
| `firmware/src/ble/ble_midi_central.h` (FMB #687) | `777c003bde` |
| `firmware/src/dsp/breaks.c` (#638/#689) | `fd44110a1c` |
| `firmware/src/dsp/breaks.h` (#638/#689) | `317ba0f6ed` |

Two important paths **differ**: FMB's `firmware/src/app/ui.c@4d2d660caa` versus #734 `ca523007dc`, and FMB's `firmware/src/app/smk_clip_keys.h@71ea5fb26a` versus #734 `d78a7dcd1b`. Treat these as newer integration-owned content. Do not overwrite them with the old FMB branch, even though both branches are labeled as ports of the same PRs.

## Popup ownership

#737 has the production mapped-page peek, current fullscreen feedback integration, live meter rendering, per-peer destination snapshots, and locked SMC PAGE movement; it has its own UI mockup and regression. This overlaps the standalone #738 mapped-page implementation. Its behavior intentionally includes a held overlay through page-peek expiry, so #738's earlier-peak supersession repair must **not** be blindly transplanted into #737: the user-visible control behavior and ownership semantics differ.

#738's unique focused extraction-based native regression and negative-test receipts are preserved at `docs/lanes/evidence/task-c/`. These checks **do not validate #737**, because they compile the distinct #738 implementation. The source comparison is **not an independent host PASS of #737**, and #737's own stated full framebuffer regression remains NOT_RUN.

## Decision under owner freeze

- Stop adding or re-porting duplicated popup feature code on #738. Retain draft as a source/test checkpoint only. It is not a second merge candidate for the same popup.
- Review the proven unique #737 implementation and independent #736 mixer FX work under their respective owners. Do not write their branches from this task.
- Do **not** merge, retarget, push to, or rewrite #734. The owner requires one fixed #734 HEAD with a posted full host suite and Elite/Pro builds **0 fail / 0 skip** before any new feature integration. No such qualification is attested in this Task C receipt.
- Source-only inspection and repeated narrow tests are not real hardware/device acceptance.

## Validation state

#738: isolated production-function regression GCC/Clang O0/O2, GCC ASan+UBSan **PASS** (27 focused assertions in each of five executions); standalone timer GCC O0/O2 **PASS** (31 checks in each). Full exact-head host suite, Elite SDK V1.2.1, Pro SDK V1.2.1, current-branch full SMC framebuffer regression: **NOT_RUN**. The frozen #734 gate remains separate.
