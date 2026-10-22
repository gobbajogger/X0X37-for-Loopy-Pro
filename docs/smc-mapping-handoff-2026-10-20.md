# SMC mapping handoff

Receiving owner: the assigned firmware integration chat.

The SMC faders use the main mixer mapping outside 909 MIX and 808 MIX. In those two pages, Bank Left selects tracks **1–8** and Bank Right selects tracks **9–11** (eleven drum tracks; engine slot 12 is kit-wide, not a twelfth drum). Faders, knobs and SELECT buttons 4–8 are inactive in the second bank. Keyboard track selection also selects the corresponding bank. Pickup resets when a target changes.

Encoders edit visible page or menu fields. In the three mixer modes, knobs trim the same destinations as the faders. Unused positions do nothing. Clip editor encoders retain their visible fields. Confirmation screens block fader and encoder edits. Help blocks encoders but leaves fader pickup running so strips can still reach their targets. The main mixer faders remain available over ordinary menus and clip editor pages.

## Input and changes

- Lane: `lane/mb-smc-dedup-12`, based on `38b3070`.
- The base includes `029c8c6`, which assigns main mixer fader 8 to the master filter. Integrate that dependency if it is absent.
- Production changes are confined to `firmware/src/app/smc_mixer.c`. Button routing tables, packed parameter comparison and one fader display refresh per event reduce flash use without removing controls or samples.
- `docs/SMC_MIXER.md` describes the controls. Host regressions cover both eleven-track kits, bank bounds and pickup reset, unique targets, menus, clip visibility, fader writes and button shortcuts. They are registered in `tests/run_tests.sh`.

## Checked

Commands ran in `/Users/mac/Documents/x0x37/smc_dedup_12`. The target build and focused tests cover the final production source. The simulator result covers its unchanged sources.

| Command | Result | Local log |
| --- | --- | --- |
| `MODEL=elite bash build.sh --board smk37` | PASS, including target build checks | `evidence/build-elite.log` |
| `X0X_TEST_JOBS=1 bash tests/run_tests.sh smc-protocol smc-fader smc-buttons smc-mapping smc-filter smc-part-select smc-clip-visibility` | All seven selected tests PASS | `evidence/smc-tests.log` |
| `X0X_HOST_DEFS=-D_DARWIN_C_SOURCE bash host/build_host.sh` | Simulator build PASS with the macOS SDK feature flag | `evidence/host-build.log` |
| `X0X_QUICK_ONLY=ste X0X_QC_BASE=HEAD bash tools/quickcheck.sh "$PWD"` | PASS, grammar advice only | `evidence/docs-check.log` |
| `git diff --check` | PASS | No output |

The target used the local JieLi toolchain and pinned AC79 SDK through Docker. Its image is 552648 bytes, leaving **24 bytes** in the 552672-byte app code area. Static RAM is 97748/98304 bytes; the pool is 335628/344064 bytes. The unchanged base built with 152 bytes of app headroom (`evidence/baseline-elite.log`). No memory limits, model guards, features or sample quality were reduced.

## Next action and limits

Integrate the checked lane and rerun the combined target build and formal gate. Flash headroom is tight. This lane has not run a formal gate, installed firmware, or tested these mappings on the device. Only the Elite target was built. The host simulator does not validate the SMC hardware transport; production SMC routing is covered by the focused host tests and target compilation.
