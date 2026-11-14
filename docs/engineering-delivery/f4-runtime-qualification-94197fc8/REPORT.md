# F4 runtime qualification at 94197fc8

The five scoped registered host selectors and the isolated supplementary driver PASS on `gobbajogger/X0X37-for-Loopy-Pro@94197fc839b00aff47b98ca02f3c6396e4a6eabf`. This qualifies the recorded production host behaviour, with the fixture boundaries below. It does not close physical acceptance or establish firmware release readiness.

Qualification artifacts are local and **NOT PUSHED**. Branch: none (immutable source archive). PR: none. No shared source, UI, firmware, registry, roadmap or owner lane was changed. The original `/workspace/X0X37-for-Loopy-Pro` checkout remains clean.

## Exact inputs and receipt recovery

- Downloaded the immutable GitHub REST tarball, checked archive traversal and relative symlink destinations, then independently verified **5,345 / 5,345 tracked blob identities**, zero mismatches, against recursive tree `72d28fbc741244d18fa3d18495da16ab0fcb1d24`. Rechecked after execution: zero tracked-source mismatches. See `source-closure.json` and `source-tree.json`.
- Consumed F4 README blob **`6f3e3a520cd2cb51fc98d69d1ee4e636df03b01a`**, its later `FX_HELD_ARROW.md`, current registrations and actual production queue/renderer callers. Canonical roadmap is retained verbatim in `canonical-ROADMAP.md` from `32b0cacd17c89f23bb2f16915732750d158b33e7`.
- First executed exact-source scenario: `SMC_UI_OUT=/workspace/f4-qualification/runs/remainder sh tests/host/smc_remainder_test.sh`, exit **0**, **205,893 checks PASS**. Environment was running and usable; neither source preparation nor tool access remains pending.
- Read later #800 and #832 evidence before selecting checks. #800's original NOT_RUN statements are historical. The November 5 v4-fix `43de100c83d845409bb801ecf29238a78f179aed` has producer, advisory QA and native-build evidence reporting **205,861 remainder checks PASS**; it also retains unrelated suite failures. #800 was subsequently closed as present on main via #864. These are evidence of later execution, not current-source results. See `later-receipts.json`, especially #832 comments **6656086291**, **6656267028**, **6656317544**, and #800 **6813759590**.
- Compared that receipt's relevant blobs against the requested source. SMC, UI, engine, host, fonts, shared fixture and several individual fixtures changed. The old receipt does not establish the current production closure; the unchanged lock/beat and mode-page fixture files alone are insufficient. `receipt-input-comparison.json` records exact old/current blobs. Only the five relevant selectors were run; no historical #807 provenance recovery or full suite.
- Live `nightly` read resolved to `66073c68dd80d51a7053432c2c161a8b720a1341`; its maintained routing permits this current delegated work. Target checkout's older classic-only instructions do not supersede the explicit Work qualification assignment. Live `coordination/merge-queue` returned GitHub **404**; no ledger or owner change was attempted.

## Executed evidence

Linux x86_64, GCC **14.2.0**, Python **3.12.14**, Pillow **12.3.0**, default generated **Barlow** fonts from this exact archive. `tools/check_original_inputs.py`: **PASS**, ten active inputs, manifest SHA-256 `9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc`. No SDK/toolchain recovery or workaround was needed for host execution.

| Check | Exit | Result | Receipt |
| --- | --- | --- | --- |
| `sh tests/host/smc_remainder_test.sh` | 0 | 205,893 checks PASS | `remainder.log` |
| `sh tests/host/smc_select_mode_test.sh` | 0 | 5,303 production-input checks PASS | `smc_select_mode.log` |
| `sh tests/host/smc_lock_beat_test.sh` | 0 | 5,141 production checks PASS | `smc_lock_beat.log` |
| `sh tests/host/smc_ui_popup_test.sh` | 0 | 4,958 checks PASS | `smc_ui_popup.log` |
| `sh tests/host/smc_mode_page_test.sh` | 0 | 1,308 checks PASS | `smc_mode_page.log` |
| Isolated `supplement.c` through unchanged production host launcher | 0 | 302 checks PASS; 16 actual `ui_frame` captures | `supplement.log`, `frames.json` |

The selectors are actually registered in `tests/run_tests.sh`: SELECT line 415, lock/beat 472, popup 495, remainder 496, mode-page 500; the generation class includes them as **G**. Commands used their unchanged direct registered launchers, serially. The supplementary command is:

```sh
SMC_UI_TEST_FILE=/workspace/f4-qualification/supplement.c \
SMC_UI_OUT=/workspace/f4-qualification/runs/supplement \
sh tests/host/smc_remainder_test.sh
```

The fixture defines `BOARD_SMK37=1` and `X0X_DEV_PADCAL=1`, with original-engine host composition. These are **production host tests**, not a native Elite/Pro build, device-accurate CPU emulation or physical-controller observation. Hardware I/O, clip field storage and clip grid painter are substitutes; clip SMC navigation functions are extracted from production `main_fm1.c`, and production SMC queue/dispatch, UI painter, engine and sequencer execute. No clip storage/hardware acceptance is inferred.

## Row dispositions

| Row | Recorded host acceptance | Remaining boundary |
| --- | --- | --- |
| #31 | PASS: first SELECT chooses part, duplicate NoteOn while held does not reselect, release/repress enters the selected kit's individual mixer for **both 909 and 808**; selected sound's next SELECT returns to main MIX. Supplement repeats this across DAW BLE, User BLE and host User input. Locked targets and keyboard state/restoration are covered by current fixtures. | Physical control/feedback acceptance remains open. First selection and touched fader target are distinct. Current mapped-page peek temporarily borrows the keyboard display; old claims of immediate keyboard display invariance must not be copied literally. |
| #37 | PASS: strip-eight modes/MIX cycle; clip PREV/NEXT versus Up/Down knob banks; 12 both-kit, MIX/SEQ clip cases assert no accidental REC/SOLO/card side effects; drum populated/empty bank precedence, locked PART/MIX identity and FX short-tap/held-arrow ownership retained. | Physical routing and full clip device/storage/painter acceptance are not established. |
| #40 | PASS: context SELECT and retained User parsing. User channels/CC mapping, host channel-16 opt-in and MMC central/USB/peripheral **software entrypoints** exercised. User host works without a central BLE link. | Literal “no User mode” is **not satisfied as a source claim**: `ui.c:494` defines `SMC_MAP_N={"DAW","USER"}`, lines 851–853 retain setting mutation and `USER: CH16 + MMC`, and line 1760 exposes the setting. Existing roadmap/SMC owner must reconcile wording with intentionally retained compatibility. No mode removal is proposed or implemented. |
| #46 | PASS: real sequencer clock at 62.5/125/250 BPM, AUTO/EXTERNAL loss and last-BPM fallback, STOP/reconnect, solid lock, unlock/reset, internal tempo and seeded uint32 sample-clock wrap. 900 ms gesture boundary, millisecond wrap, loss and guard cases pass. Supplement captures actual audio-driven pickup light/dim, external WAIT/tick/loss and lock/stopped states without faking a physical fader touch. | Output is software LED projection and host framebuffer. Physical lamp identity, native timing/RAM and device behaviour remain unclaimed. |
| #67 | PASS: real physical-note ownership retains popup beyond 4,800 ms, release+1,199 ms remains active and **release+1,200 ms expires**, including uint32 millisecond wrap, on DAW BLE/User BLE/host User routes. Help, confirmation and keyboard navigation take ownership; current popup fixture covers views and modal handling. | Default **one peer** only. Optional two-peer mode and physical link behaviour were not required or rerun. |

## Actual renders inspected

`contact-sheet.png` contains sixteen unscaled **240×240** screenshots, with labels outside the original pixels. `runs/supplement/*.png` are generated through actual **`ui_frame()`** after production input; `frames.json` pins each image. Inspected the eight-strip grid, independent knob-bank captions, both kit mixer banks, pickup header/direction, stopped/locked screen, external loss state, held FX arrow edit and release.

The held-FX scene changes its live footer from **REV THROW** to **DLY THROW** after Right, with `FX+<>` and an effect picture while held; release ends the effect and retains the edited mapping. This is the current source's actual presentation, not a new design approval. The older RC1 footer mismatch/proposal records are retained separately; no broad redesign or #891 implementation was undertaken.

`runs/renderer-diagnostic/` contains an earlier separate set drawn by the production popup painter directly. Those are diagnostic renderer outputs, **not actual displayed mapped-page-peek frames**. The final inspected set is the `ui_frame` set above. No native screenshot, device capture or physical lamp claim is made.

## Independently confirmed issue and next owner action

**Retained optional capture-fixture bounds defect:** unchanged `tests/host/smc_remainder_test.c:469,475,476` uses patterns **63/62**, including `proj.pat[63].bass[...]`, while `firmware/src/seq/pattern.h:12` defines **NPAT=32** and `firmware/src/app/x0x.h:121` stores `pat[NPAT]`. The supplementary runtime independently asserts the actual array length is 32. The optional `smc_remainder_capture.c` calls this function; ordinary registered remainder main does **not** call it, explaining why the current registered test passes. This is the same issue class observed in #800 comment **6557130236**, confirmed on this exact current source. The unsafe capture function was not executed.

Existing F4/SMC **test owner**: correct only the capture scene's invalid high-pattern constants to valid intended boundary patterns, then run that optional capture driver with bounds diagnostics and inspect its resulting images. Preserve registered behavioural assertions and held-FX decisions. This qualification lane supplies `supplement.c` as a safe alternative, with all source ownership unchanged.

The new supplementary driver's two initial failed assumptions are preserved, not hidden: `supplement-initial.c/.log` assumed keyboard invariance across fader peek plus navigation; `supplement-clock-initial.c/.log` assumed WAIT before its half-second deadline. Source tracing confirmed current mapped-page peek and `seq_clock_waiting` semantics; only this new owned fixture was corrected. Final `supplement.c` checks restoration after the real 1,200 ms peek and waits `FS/2+1` rendered samples for WAIT. No registered assertion or production behaviour was weakened or changed. See `fixture-development.md`.

## Delivery and continuation

Return `NEXT_STAGE.json` and this report to creator thread **[private creator identity retained in Library]** for the incumbent SMC test owner, firmware integrator and roadmap owner. No evidence was published externally under the task's no-publication limit. Delivery to creator is the supported route; incumbent owner consumption is **not yet verified**.

Standard `thread_management.create_thread` / `read_thread` and saved-configuration management tools were searched in the actual registry and are absent. Environment status does expose verified configuration identity, recorded in `NEXT_STAGE.json`, but it is not a thread creator. **No successor was created or admitted**, and no browser creation or uncertain-admission retry was attempted.

This bounded host qualification is complete. Existing owners retain the capture-fixture repair, #40 wording reconciliation, and separately authorized physical acceptance. Full host suite, native builds, device-accurate emulator, hardware, merge and publication were **NOT_RUN / NOT_PERFORMED by scope**, not failed availability probes. No independent ready runtime scenario identified in the requested five-row scope remains unexecuted.
