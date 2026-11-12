# Independent SMC SELECT LED review

Verdict: **PASS**, no blocking finding at `5a7c1df58c0e09fa553a9ab3a750c2754208a29e`, tree `2cf4f9604dc982272985433e461578c33eefc638`. Base `e2b17c9c98b41f0b9c3f5ecdac73df0d2ebb304d`; production source blob `36d8018012e0c38fd79a0bf0e333d33c1435fb60`.

Production change is confined to smc_led's read-only SELECT source-strip return logic. Outside the actual FX context, LEDs follow ordinary ui.part selection. Inside V_FX with no overlay/clip mode, source buses use bits0..4 directly, correcting the prior missing303B and misindexed BREAK indicators. Existing drum sound-bank, target-lock and main-strip navigation precedence remains. No press, release, mutation or engine path changed in this follow-up.

The extended test captures actual production smc_frame NoteOn output through a synthetic transport boundary and also checks smc_led state. It exercises ordinary modes/source parts, drum banks including the partial final bank, FX source holds, main pending/held/released navigation, and mode transitions. Existing input/ownership assertions remain. The negative runner preserves all prior default mutations, adds a named LED mutation, rejects unknown selectors and requires runtime assertion failures rather than compile failures.

Independent checks, all expected results observed:
- `sh tests/host/smc_select_mode_test.sh`: **5166PASS**, exit0.
- `python3 tests/host/smc_interaction_negative_test.py select-led-fx-only`: PASS, the FX-only return mutation causes a real SELECT runtime assertion failure.
- `python3 tests/host/smc_filter_test.py`: PASS, independent master mute/filter extraction retained.
- New test against exact old e2b17c9c source: **expected runtime FAIL**, exit1 at SELECT LED assertion line19; compilation succeeded.
- `git diff --check e2b17c9c..5a7c1df`: PASS.

Exact commands, environments, hashes and log digests are in receipt.json. Host compile retains the pre-existing CK_FN redefinition warning, without compile/runtime failure. No source/test edits, Git mutation, GUI or device action were performed by this reviewer.

## Qualification still required

The prior94061cf/e2b17c9c native result does **not** qualify this changed production source. Refresh applicable host evidence on this source (select-mode, filter, reachability, lock, UI and interaction-negative); confirm actual selector execution. Rebuild affected Elite and Pro development apps and model packages with current verified build inputs. Unchanged controller reuse requires complete closure and hash/CRC verification, followed by exact package composition and validation. Record current model, firmware/controller identities and code/RAM/stack/pool budgets, retaining pool_free >=8192. Do not add standalone savings or relabel old packages. This review is source/host evidence only; physical lamp identity and device behavior remain unverified. No flash, publication or promotion follows from this result.

## Final feedback fixture closure

Independent follow-up verdict: **PASS** at `2ed626fcb0cb2c1ce1d7ff85ecfbe4e6acb6b1ca`, tree `78aeb867d2dc70fa670d4d63759ff163bd3d3059`. Relative to reviewed5a7c1df, only `tests/host/smc_feedback_test.py` changed; the firmware diff is empty and production blob remains `36d8018012e0c38fd79a0bf0e333d33c1435fb60`.

The fixture continues to compile actual smc_led/smc_frame functions verbatim. It corrects old HOME-as-FX expectations and expands separate realFX source-bit assertions. Emitted-frame checks now require selected909 lit, a909→BREAK switch emitting both old-off/new-on,303B FX hold/release on its own note, and leavingFX restoring the selected source. The43initial-note frame count, duplicate-frame suppression, transport, connection/owner refusals and fader output assertions remain. No safety assertion or route was removed.

Focused independent execution: `python3 tests/host/smc_feedback_test.py` **1919PASS**, exit0. The same final fixture against exact e2b17c9c production rejects the old LED logic through actual runtime state/wire failures and frame-count assertion, rather than a compilation failure. `git diff --check 5a7c1df..2ed626fc` passes. Details and hashes are in final-feedback-receipt.json; logs are feedback-final.log and feedback-old-led-negative.log.

The prior5166 production-input/frame evidence remains valid under unchanged source and unchanged SELECT test. Native qualification must still bind the current corrected test/input closure; this host-only review makes no native or hardware readiness claim. Reviewer made no source/test edits or Git mutations.
