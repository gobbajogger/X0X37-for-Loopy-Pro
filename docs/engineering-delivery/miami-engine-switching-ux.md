# Miami engine switching UX (clarity pass)

Reason: operators must change between sound engines (native303 ↔ WBL1)
without ambiguous labels or silent state.
Purpose: record the acceptance bar, the ambiguities found on the Miami pin
and stream5 tip, and the minimal clarity fixes that do not rewrite schema.
User effect: when a bass step owns a sound snapshot, the knob strip shows
which engine that snapshot uses (`303` or `WBL1`), distinct from the part
name `303A`/`303B`.

## Pins

| Item | Value |
| --- | --- |
| Product base (this branch) | stream5 tip `fabd8b14` / PR #259 |
| Miami runtime pin | `d7f156ec` on `experiment/miami-step-snapshot-20261026` |
| Schema ownership | sole REC/Miami writers; this change does not touch stream layout |

## Acceptance bar

Seamless engine change with no UX/UI ambiguities. That requires a public
select/switch affordance, a visible active-engine indicator, and honest
error/empty states. **Not yet PASS** for end-to-end switch: there is still
no operator menu or API to change the live backend.

## Ambiguities (audit)

1. **No public switch.** Miami DEV fixture hardwires part A to WBL1 at boot
   (`X0X_DEV_WOBBLE`); stream5 stores identities but has no wobble runtime.
2. **Part name vs engine identity.** Labels stay `303A`/`303B` even when the
   step sound is WBL1 — easy to read as “always 303”.
3. **Silent snapshot identity.** Before this pass, holding a step never
   showed whether a snapshot existed or which engine it named.
4. **Identity mismatch is silent.** Miami runtime ignores a snapshot whose
   identity does not match the active backend; UI gave no toast.
5. **Load failures collapse.** Mixed/invalid stream4 and empty/corrupt loads
   all say `NO VALID SAVED PROJECT` — no layout-specific wording.
6. **Empty stream4.** Schema treats empty-4 as either layout and next save
   writes 5; operators get no migrate notice.
7. **Knob names stay 303.** WBL1 has different parameters; the strip still
   uses native303 names when only storage (not runtime) is present.
8. **Frozen UI SHA missing.** Documented piano-roll visual pin
   `7ca652b5…` is not on the GitHub remote.

## Shipped here (low-risk clarity)

- `motion_snapshot_engine_label()` — stable short names `303` / `WBL1`.
- Knob-strip tag appends ` · 303` or ` · WBL1` when the held/p-locked bass
  step has a `MOT_SNAPSHOT` record.
- Host check in `snapshot_storage_test` for the label helper.

No schema rewrite, no PE #246 grafts, no host-as-device PASS, no flash /
NIGHTLY / Pages.

## Deferred (needed for seamless PASS)

- Public select/switch control with visible active-engine state per part.
- DEV_WOBBLE boot fixture must not silently replace part A without a label.
- Distinct load toasts for layout reject vs CRC vs empty (needs return codes
  owned by schema writers).
- Optional one-line migrate notice when stream4 → 5.
- Engine-specific knob names when WBL1 is active.
- Import of accepted piano-roll visuals once the UI source is available.

## How to verify

1. `sh tests/host/run_snapshot_storage.sh` (label helper + storage).
2. `sh tests/host/run_stream5_combined.sh` (schema unchanged).
3. On device/UI sim: capture or load a step snapshot with identity 2; hold
   that step on SEQ — tag reads `303A STEP n · WBL1`. Identity 1 reads
   `· 303`. No snapshot — tag unchanged.
4. Confirm there is still **no** menu item that switches the live engine
   (expected until the deferred switch lands).
