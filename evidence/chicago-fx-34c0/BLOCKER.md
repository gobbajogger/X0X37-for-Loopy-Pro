# FCDC-205 Chicago Fx — honest blocker evidence

## Decision

**NO_FIRMWARE_DELIVERABLE.** Product Chicago Fx Type / Fx Edit 1 / Fx Edit 2
fidelity cannot land without a large insert-DSP / XIP program. This tip is
docs + evidence only.

Canonical note:
`docs/engineering-delivery/chicago-fx-fidelity-blocker.md`

## Why not implement

1. Chicago strip is per-voice **insert**; product has shared `T_FX` fxbus +
   `T_MIX` sends only (`firmware/src/dsp/fxbus.{h,c}`, `engine.h`).
2. Ring Mod, Cho/Flg, Ensemble, Phaser, Gate Reverb have **no** DSP in tree;
   silent stubs refused.
3. Live WBL1 already XIP FAIL (~2.9–3.3 KB class); insert FX family will not
   fit as a param-wire PR. Controller flash margin historically ~44 B.
4. After `#281` on stream5 (`a692adfd`), `BASS303_NPARAMS` is 23 and
   `NPARAMS_MAX` is 24 — only **one** free slot (index 23). Three Fx pots need
   a further schema bump; not done here for stubs.
5. Miami wobble remains **WBL1 / XIP blocked** — not faked onto native303.

## Diff scope

| Path | Role |
| --- | --- |
| `docs/engineering-delivery/chicago-fx-fidelity-blocker.md` | Blocker decision |
| `evidence/chicago-fx-34c0/BLOCKER.md` | This packet |

No `firmware/`, `tests/host/`, or schema edits.

## Local proof

Docs-only tip. Plan with `tools/local_proof_plan.py` from `origin/main` (tool
not yet on stream5 base). Expected kind: docs-only — no firmware BF suite,
no package rebuild.

```bash
python3 /tmp/local_proof_plan.py \
  --repo "$PWD" \
  --base origin/cursor/stream4-combined-schema-53a1 \
  --head HEAD \
  --branch cursor/chicago-fx-34c0
```

Record planner JSON and any required selectors under this directory after the
commit exists.

## Parent PR body (paste)

```
FCDC-205 Chicago Fx Type / Edit 1 / Edit 2 — honest fidelity blocker

Reason: Chicago AU exposes a 12-type per-voice insert strip; X0X37 has a
shared send fxbus only. Stub pots or silent global remaps would mislead players.
Purpose: Record the architecture/XIP/schema blockers so the lane does not
fake fidelity.
End user: Bank tables keep Fx names; the device does not claim insert FX it
cannot run.

BLOCKED: no BASS303_FX* pots. Resume needs a flash/XIP insert program or an
explicit labeled send-FX product decision after amp ADSR + NPARAMS headroom.

Miami wobble: confirmed WBL1/XIP blocked; not on native303.

Evidence: docs/engineering-delivery/chicago-fx-fidelity-blocker.md
         evidence/chicago-fx-34c0/BLOCKER.md
```

## Local proof result

Planner (from `origin/main` copy; tool not on stream5 tip):

- kind: `evidence-only`
- `x0x_tests`: []
- `rebuild_packages`: false
- verify: scope/whitespace `--check` clean; tip claims match docs/evidence only
- skip: publish/web kitchen sink, `test_web.mjs`, full `tests/run_tests.sh`

JSON: `evidence/chicago-fx-34c0/local_proof_plan.json`

```
git diff --check origin/cursor/stream4-combined-schema-53a1...HEAD  # exit 0
```

No firmware host suite required for docs/evidence tip. `X0X_TEST_JOBS=8` unused (empty selector list).
