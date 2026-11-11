## Continuation on the latest integrated build (2026-10-30)

**Current candidate integration base (merge parent):** `wip/one-build` at `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`. This is newer than the original `a0279524` UI base and already contains the existing freeplay Miami rate-pad source, SMC and BREAK integration, wrap fixes and approved UI. The original base and PR #701 below remain historical provenance, not this candidate's parent. The exact 51-file PR #701 delta is grafted without collision onto the `one-build` tree. No production `ui.c`, HAL, engine management, sound metadata, font, boot or controller file from the current integrated base is overwritten. The source-closure check pins the *current* UI and selector blobs and computes the safety diff relative to `cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21`.

Preserved current integrated base features: existing eight freeplay wobble rate-pad selectors, engine and preset SOUND row, original program names, all six palettes/glyph families, SMC and BREAK source. The separately owned shape-pad implementation is not present and is not claimed. Active-voice CPU improvement beyond already imported optimizations is deferred to independent bit-exact work; this composition only integrates the proven optimization source.

**Review plan:** check exact candidate host render, original factory/UI selector paths, rate-pad ownership, Chicago/Miami host golden and state, BLE/SMC and BREAK regressions, Elite/Pro native budgets and device PERF. Run `python3 tests/host/test_original_build_composition.py` and `python3 tests/host/test_freeplay_rate_pad_port.py` alongside the original plan's host checks, without enabling legacy DSP. Run the selected original profile with `X0X_ORIGINAL_SYNTH=1 X0X_WOBBLE=0 X0X_DEV_WOBBLE=1` for freeplay pads.

**No device operation, release or publication is included.** Existing user-reported PERF percentages from another firmware are not measurements of this SHA.

---

# DnBOX Chicago/Miami and approved UI integration

## Scope and immutable ownership

Owner-confirmed base: `wip/ui-theme` at `a0279524decd54d2840474883fefa5882b15fb4d`, directly continued on `wip/engines-ui-one-build`.
Use the base's **existing** `firmware/src/app/ui.c`, `main_fm1.c`, `boot_screen.h`, fonts, six palettes, original-control metadata, and matrix-B styling without replacing them with the older performance-build UI. The full approved glyph/screen material remains from the UI lane. No UI behavior is reimplemented from scratch.

Import the existing optimized Chicago files **as complete Git blobs** from `wip/chicago-opt` at `dd735533eaceae5856f5b657fe617667e7cc5b6e`; import the optimized Miami files from `wip/miami-opt2` at `c305351b73b8821ded0b59277648cb0191f10a2f`. Every executable import must be the exact original blob. The source gate pins the affected engine files, selector, approved UI, and original-profile backend. `chicago_selector.c` is already selected by `ORIGINAL_SYNTH_UNITS` in `tools/build.py`, and `engine.h` enables tagged sound / preset preview in original mode. Engine/preset editing in the device renderer uses `R_BENGINE`/`R_BPRESET`, `engine_bass_select_preset()`, `build_pages()` and requested/active/pending state. The `G_ENGSEL`, `PRESET` and `RELOAD` names in `web/EDITOR_PROTOCOL.md` document an older Felucca/web protocol; they are **not** proof that the corresponding SysEx commands are answered by this device's `app/editor.c`.

No `bass303.c` or `wobble.c` is linked in the selected original profile. Only project revision 13 is admitted. No private #684 headers, SDK/toolchain, `.img`, `.fwsc` or other product binaries belong in the Git diff. Do not change the bootloader or protected flash regions.

The reported hardware performance build `c287cfe4` (Chicago ~0%, Miami ~0.2% *idle* as reported by owner) is **not** an accessible commit in this GitHub repository and has not been independently compared or rebuilt. None of its reported load measurements apply to this new integration without a matching device build/test.

### Rendering and screen inventory

The candidate inherits the base's shared fonts, header material, parameter glyph dispatcher, safe preset/engine SOUND row, physical K5–K8 captions, Light palette added after the original five palettes, and static DnBOX boot. **Production UI code files are unchanged by the engine imports**, so these screens are *inherited*, not newly redrawn in this lane.

The added `ui_theme_test.c` production-renderer capture matrix covers these **22 screen/view cases under all six palettes** (132 additional captures):

- HOME; 909 SOUND; 808 SOUND; Chicago SOUND; Chicago engine/preset selector; Miami SOUND; Miami engine/preset selector; BREAK.
- SEQ 909, 808, Chicago, Miami, BREAK; TB-3PO GENERATE; FX.
- MIX PARTS, 909, 808, MASTER; SONG; GLOBAL/settings; PERF.

The inherited original test also exercises both original engine matrices, every factory name (55 Chicago / 30 Miami), pending/cancel, undo, lock/motion, wave glyphs, the actual boot screen and row/selector transition states. Capture and nonmutation tests **must be executed** on this exact candidate; a recorded mockup, this inventory, or a UI source review does not prove unclipped actual rendering.

The preset display must show the original factory name and distinguish pending, active and edited states. The retained SOUND entry is the last original-engine matrix row and must work on both 303 slots, not only on the first page. Do not silently remap K4's established preset behavior. Keep the source-of-truth engine/control names from `original_sound.c`, not the retired bass303/wobble enumerations.

### Features owned by other lanes, not reimplemented here

The existing **wobble rate selector on eight freeplay pads** is present on `wip/one-build` at `c32f92771707965a60ba9f18bf8e428f6afbfce5` (look for `division_pad_owner/index/label/input` in `ui.c`). It is not yet integrated in the owner-selected `a0279524` base. The four **shape pads** are assigned to the separate shape-pads feature thread and are likewise not implemented or claimed in this lane. Once those owners deliver reviewed exact source with no pad collisions, the sole integrator can compose them into the nominated unified build.

Active-voice CPU improvements beyond these **already implemented** Chicago and Miami lane blobs remain separate bit-exact optimization work. No new active-voice shortcut is invented in this merge.

## Required Mac proof on this source commit

All commands start in the clean checkout of the **pushed full `wip/engines-ui-one-build` SHA**, with the owner's exact private build inputs installed locally and **never committed**. Keep the same golden render directory used by the established original-engine lanes. Stop if the active-input check reports missing inputs or generated stubs.

```sh
git fetch origin wip/engines-ui-one-build
git checkout wip/engines-ui-one-build
git pull --ff-only origin wip/engines-ui-one-build
git rev-parse HEAD
python3 -B tests/host/test_engines_ui_source_closure.py
python3 tools/check_original_inputs.py --scope active --json
sh tests/host/original_release_ui_test.sh build/engines-ui-original-ui
ORIGINAL_UI_TEST_FILE=tests/host/ui_theme_test.c sh tests/host/original_release_ui_test.sh build/engines-ui-six-theme
sh tests/host/original_release_integration_test.sh build/engines-ui-integration
sh tests/host/original_idle_test.sh build/engines-ui-idle
sh tests/host/run_chicago_clock_span.sh build/engines-ui-clock
sh tests/host/run_chicago_chorus_span.sh build/engines-ui-chorus
python3 tests/host/miami_cpu_test.py --out build/engines-ui-miami-all30
python3.12 tools/synth684_harness/run.py --golden build/synth684-golden-9d1cf9fb --out build/engines-ui-golden
```

**Golden acceptance:** 15/15 exact stereo rendered files and complete sample/state comparisons, not merely similar audio or stub-generated output. Cover Chicago + Miami played and released voices, multiple factory patches, long idle-then-note and rapid selector/preset adoption. A failing current-candidate comparison is a FAIL; do not suppress it. `BASE_REF` for the earlier Chicago idle-then-note differential requires its exact local baseline object:

```sh
BASE_REF=9621d6cc7be6c333462771a30bf8e98d0701f9fd CLEAN_TRANSCRIPTS=1 python3 tests/host/run_chicago_idle_then_note.py build/engines-ui-idle-note
```

For **active and idle instruction counts** run on the same qualified x86-64 Linux/Valgrind environment (for example the owner's configured Colima worker). Mac host `valgrind` itself is usually not a native Apple Silicon executable. Use the actual configured runner and preserve its flags and reference/source identity. Both non-idle program cases and idle blocks are required:

```sh
python3 tools/synth684_harness/run.py --icount --golden build/synth684-golden-9d1cf9fb --out build/engines-ui-active-icount
python3 tools/synth684_harness/run.py --icount --idle --warmup 1024 --blocks 64 --out build/engines-ui-idle-icount
```

The `--icount` command must execute **inside a Linux worker with Valgrind**. Record real per-case `ICOUNT` output and compare with the same toolchain, flags and golden baseline. CPU percentages reported by the on-device **PERF** screen also require the **exact checked package** installed on hardware. Report Chicago idle/active and Miami idle/active separately, including peak CPU and visible UI frame cadence.

### Development Elite and Pro builds, guarded install

The Mac needs SDK **AC79NN V1.2.1** and the pinned JieLi compiler. V1.2.0 is not an acceptable substitute. The normal native target build commands, once the exact current source and private inputs are available:

```sh
export JIELI_TOOLCHAIN="$HOME/.jieli/toolchain"
export AC79_SDK="$HOME/fw-AC79_AIoT_SDK"
X0X_DEV_PADCAL=1 BOARD=smk37 MODEL=elite sh build.sh
X0X_DEV_PADCAL=1 BOARD=smk37 MODEL=pro sh build.sh
```

Use the maintained scoped DEV validation runner with the committed `docs/lanes/engines-ui-dev-plan.json`, which runs the selected host checks and model-specific builds and produces provenance, budgets, package and controller hashes. Paths below are the documented Mac+Linux-worker configuration, **not evidence they ran in this thread**:

```sh
python3 -B tools/dev_validate.py run --commit "$(git rev-parse HEAD)" --model elite --plan docs/lanes/engines-ui-dev-plan.json --output build/engines-ui-dev-elite --worker-image x0x37-validation:local
python3 -B tools/dev_validate.py run --commit "$(git rev-parse HEAD)" --model pro --plan docs/lanes/engines-ui-dev-plan.json --output build/engines-ui-dev-pro --worker-image x0x37-validation:local
python3 -B tools/dev_validate.py verify --commit "$(git rev-parse HEAD)" --record build/engines-ui-dev-elite/record.json --fwsc build/engines-ui-dev-elite/x0x-smk37-elite.fwsc --ctl build/engines-ui-dev-elite/ctl.img
python3 -B tools/dev_validate.py verify --commit "$(git rev-parse HEAD)" --record build/engines-ui-dev-pro/record.json --fwsc build/engines-ui-dev-pro/x0x-smk37.fwsc --ctl build/engines-ui-dev-pro/ctl.img
```

Only **after** DEV CHECKED for the exact model/source, matching controller check, preserved sample-slot backup, approved hardware operation and owner safety checks does the designated Mac installer use the guarded foreground commands:

```sh
python3 tools/fm1_install.py --info
python3 tools/fm1_install.py build/engines-ui-dev-elite/x0x-smk37-elite.fwsc --commit "$(git rev-parse HEAD)" --config dev --dev-record build/engines-ui-dev-elite/record.json --ctl build/engines-ui-dev-elite/ctl.img --backup build/engines-ui-elite-before.x0xslots
# For SMK-37 Pro, separately, on the matching device:
python3 tools/fm1_install.py build/engines-ui-dev-pro/x0x-smk37.fwsc --commit "$(git rev-parse HEAD)" --config dev --dev-record build/engines-ui-dev-pro/record.json --ctl build/engines-ui-dev-pro/ctl.img --backup build/engines-ui-pro-before.x0xslots
```

The installer itself must enforce bootloader/identity/controller/backup guards. **Never** use `--force`, skip sample backup or run a device rescue. Neither a PR nor a successful app-only build authorizes flashing.

## Evidence and handoff

- **Source identity:** exact imported file blobs from existing WIP lanes; source-closure gate verifies expected hashes and compile-unit plan.
- **This thread:** remote Git sources inspected; Git object/tree composition and test-plan authoring. No complete current-source checkout or private V1.2.1 SDK build was verified in this execution environment; native current-head tests, bit-exact golden renders, actual screen captures, Callgrind, install previews and device tests remain **NOT RUN** until measured.
- **Draft PR only**. No merge to `nightly`/`main`, no installer/gh-pages publishing, no flash, no user sample changes.
- Receiving owner: existing independent UI/DSP QA, then sole firmware integrator. Check the full combined candidate's controls, graphics and active-voice/idle audio against the previous source/golden, then accept/reject the exact commit. The rate and shape pad owners retain separate delivery responsibility.
