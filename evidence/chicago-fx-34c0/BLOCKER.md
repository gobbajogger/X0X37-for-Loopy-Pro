# FCDC-205 Chicago Fx — SEND FX bridge evidence

## Decision

**SEND_FX_BRIDGE_SHIPPED** (labeled approximation). Not insert fidelity.
Canonical note: `docs/engineering-delivery/chicago-fx-fidelity-blocker.md`

## Honesty

- Supported: Off, Comp, Crush, Short Delay, BPM Delay, Hall → Mix + fxbus.
- NOT_YET: EQ, Ring, Cho, Ens, Phaser, GateRev → `NO INSERT` + clear sends.
- Off inactive on load (does not wipe factory 303 Mix defaults).
- Miami WBL1 unchanged / not faked onto Open303.

## Diff scope (product)

| Path | Role |
| --- | --- |
| `firmware/src/app/chicago_fx.h` | Type enum, map, factory decode |
| `firmware/src/app/engine.{h,c}` | NPARAMS_MAX 28; CHI_FX param surface |
| `firmware/src/app/x0x.h` | PROJ_SND_REV 8 |
| `firmware/src/app/project.c` | rev&lt;8 → Off defaults |
| `firmware/src/app/ui.c` | SEND FX page + commit to Mix/fxbus |
| `tests/host/chicago_fx_bridge_test.c` | Host map / NOT_YET proof |
| `tests/host/run_chicago_fx_bridge.sh` | Runner |
| `tests/run_tests.sh` | `chicago-fx-bridge` selector |
| `docs/engineering-delivery/chicago-fx-fidelity-blocker.md` | Honesty + gaps |

## Local proof

```bash
python3 /tmp/local_proof_plan.py \
  --repo "$PWD" \
  --base origin/cursor/stream4-combined-schema-53a1 \
  --head HEAD \
  --branch cursor/chicago-fx-34c0
X0X_TEST_JOBS=8 tests/run_tests.sh <selectors from plan>
```

Record planner JSON under this directory after the commit exists.
