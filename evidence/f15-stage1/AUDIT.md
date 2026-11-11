# Stage 1 audit of validated source 092589b8 (F15 after F13 LX-73 LIVE_PAGE_MATCH)

Audited tree (primary): `092589b84fd455355216cb0de93c4782236f7524` — validated / published LX-73 / BF-012 source with LIVE_PAGE_MATCH `nightly-092589b8-dev`.
Git `source_tree`: `9382fb1276e320d9f9d290dbad187fa08cdd224c`.
Content tree (non-`.md`, Stage-4/7 DEV CHECKED): `25ada1f02e2e289072fab8db464b715e09f9c0712bded561af008bc88d8c54af`.
Firmware tree: `a5ba08046700be5cdf2751b7afc1197411b24121`.
Cross-check tip: `origin/nightly` advances past this SHA (BF-013 Safe Merge `b48cd800`, tip packages). Workflow still audits the named validated/published SHA. Parallel F14 Stage 7 may republish `b48cd800`; this audit re-verified live `latest` still `nightly-092589b8-dev` (see live-verify.log). This thread does not claim Stage 7.
Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
Branch that contains this record: `cursor/f15-f03-slotprobe-stage1-237f`.
This thread: `bc-6c51c5fd-d995-54b7-baf7-b5033919009d` (F15 Stage 1; Project LX-96).
Prior cycle audited / published SHA: `092589b8…` (F13 LX-73 / BF-012 packages). Do not reopen BF-001–BF-012 or LX-73 package publish. BF-013 is already contained in this SHA’s ancestry / later nightly merges and is not re-selected.

Repair commits already in tip (not re-opened): `d11b74fc` (BF-001), `ab90f47c` (BF-002), `4f18983a` (F03 source intent), `1325f5d3` (BF-003), `6df0f81f` (FP-001), `ce690772` (BF-004), `94f4a012` (BF-005), `37bab7d1` (BF-006), `78a0452e` (BF-007), `de11b6d7` (BF-008), BF-012 / `#308` / `092589b8`, and LX-73 package publish itself.

No production source was edited. Production `main` and production GitHub Pages were not updated. No device was flashed. Firmware `nightly` was not force-pushed. Do not steal Arp/Fx sole-owner leftovers (`bc-8efb2f94…`).

## Published page (re-verified this audit)

Installer: https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/

| Check | Result |
| --- | --- |
| `latest` in live catalog | `nightly-092589b8-dev` |
| Label / `hardware_tested` | `DEV / HARDWARE TEST — DEVICE NOT TESTED` / false |
| `source_commit` / `source_tree` | `092589b8…` / `9382fb12…` **MATCH** handed SHA |
| Elite bundled re-fetch | size 651396, sha256 `3d107710cde229d7faf3e007ae14e324d7ea5ce209a802642dacf40608b2895e` **MATCH** |
| Pro bundled re-fetch | size 651392, sha256 `60dc7fda13fdaa3ee456736588e18b2d9e7e50cc34a800504bde678e14ba12d3` **MATCH** |
| `controller_sha256` (bundled) | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` |
| Result | **LIVE_PAGE_MATCH** (page has not moved to `b48cd800` yet) |

Log: `evidence/f15-stage1/live-verify.log`.

Cited blobs at `092589b8`:

| File | Blob |
| --- | --- |
| `tests/install_test.py` | `4de1cc02719fd606aafce4c78537a844d925dbda` (**BF-034 baseline**; FakeSMK empty/`answer_slots` broken) |
| `tests/install_f03_slot_probe_test.py` | `018d80e6fb871c337c680bc233de0d159391b95e` |
| `tools/fm1_install.py` | `3195c4b1249648074d15bd613db57067a16642e5` |
| `tools/x0x_slots.py` | `f05a978ce01641bf75d44be6320eae494a1f994b` (`parse_info` rejects count/size 0) |

Host witness (this audit): `python3 tests/install_f03_slot_probe_test.py` **FAIL** exit 1 — mute path does not raise; empty FakeSMK path raises `SlotError` and aborts. Log: `bf034-f03-slotprobe-witness.log`. Supporting `tests/install_test.py` **FAIL** (9), including missing/malformed SMP_INFO fail-closed cases: `install-test-supporting.log`. `X0X_TEST_JOBS=8`. Actions ≠ proof.

## Selected defect

**BF-034. F03 / installer slot-probe fail-closed host negatives FAIL on tip (`tests/install_test.py` FakeSMK + `tests/install_f03_slot_probe_test.py`).**

Selection order (`docs/rules/workflow.md` Next cycle):

1. Oldest deferred not fixed in `092589b8` (`evidence/f13-stage2/DEFERRED.md`): withheld asset cache, data-safety lanes, HTML `find(IS_OTA)`, longer `SLOT_PROBE`, delete `make_site.py`, nightly second qualification field, F06 wording — all excluded / design / optional / decision-open / docs debt — **none are tip source FAILs**.
2. Recorded failing check not already fixed: Stage-7 safety log + Codex on #327 → `install_f03_slot_probe_test` / installer fail-closed negatives. **Local proof: still FAIL on audited SHA.** Selected.
3. Older open BF findings already contained in this SHA (BF-001–BF-013 and later landed BF merges) are not reopened.

## Evidence

Definition: `evidence/f15-stage1/bf034-definition.md`.

FakeSMK early CMD_INFO shortcut replies `[0,0]` (invalid for `parse_info`); `answer_slots=False` still serves `slot_info`. Mute never times out; “empty layout” is not a successful empty parse; `slot_info=None` mute for install_test is shadowed. Stage-7 labeled this SUPPORTING_FAIL; this audit does not bury it — local exit 1 is the gate.

## Affected scope

- `tests/install_test.py` — FakeSMK SMP_INFO answering: mute must silence all INFO replies; successful empty must use valid empty descriptors (not count/size 0); `slot_info=None` must timeout.
- `tests/install_f03_slot_probe_test.py` — only if assertions need alignment after harness fix (prefer fix harness to match existing F03 contract).
- `tools/fm1_install.py` / `tools/x0x_slots.py` — only if a tip production fail-closed gap remains after harness repair (witness first; do not change production paths without a failing production-path case).
- Out of scope: production `main`/Pages, flash, BF-001–BF-013, FP-001, LX-73 republish, Arp/Fx sole-owner leftovers, lengthening `SLOT_PROBE` budgets, force-push `nightly`.

## Proposed fix

**Decision for this cycle: repair BF-034 only.**

1. Fix `FakeSMK.rx` so `answer_slots=False` and `slot_info=None` produce no SMP_INFO answer (timeout).
2. Remove or replace the `[0,0]` “empty” shortcut with a valid empty layout reply (or rely on default empty `slot_info`).
3. Keep fail-closed: probe failure → no write / exit 7; successful empty → no backup required.
4. Host: `install_f03_slot_probe_test.py` exit 0; restore affected install_test fail-closed negatives. `X0X_TEST_JOBS≥8`.
5. Implement on new `cursor/<slug>-stage2-237f` from pushed tip of `cursor/f15-f03-slotprobe-stage1-237f`. `X0X_DEV_PADCAL=1` if any firmware rebuild later.

## Approvals

Workflow + deferred-order rule select this recorded failing check after deferred exhaustion without asking. Standing owner pre-approval covers properly tested Project merges. This stage does not merge production `main`, publish production Pages, or flash.

Approved for Stage 2: BF-034 FakeSMK / F03 slot-probe fail-closed host repair as above.

Not approved in this repair: deferred non-FAIL list items, reopening BF-001–BF-013 / FP-001, Arp/Fx leftovers, force-push nightly, production Pages, LX-73 package work.

## Missing decisions

None block BF-034. Optional later: whether longer `SLOT_PROBE` budgets deserve a separate hardening cycle; Felucca `make_site.py` full delete.

## Defects considered and not selected

### Already fixed / not reopen

| Item | Disposition |
| --- | --- |
| BF-001–BF-012, FP-001, LX-73 publish | Present / published. Closed. |
| BF-013 | Contained in SHA ancestry / parallel F14; not tip FAIL on `fxbus` at `092589b8` (`>= 20.0f` already). |

### Deferred list (`evidence/f13-stage2/DEFERRED.md`)

| Item | Disposition |
| --- | --- |
| All listed deferred items | Not tip FAILs; do not select ahead of BF-034. |

### Recorded failing checks

| Item | Disposition |
| --- | --- |
| `install_f03_slot_probe_test` + installer fail-closed negatives | **SELECTED** (BF-034). Local FAIL. |
| GitHub Actions / Codex-on-GitHub | Never proof. Codex used only as finding pointer; local witness is the gate. |

### Explicitly not stolen

Arp/Fx sole-owner `bc-8efb2f94…`. Parallel F14 Stage 7 tip publish for `b48cd800` — out of this Stage 1; live page still `092589b8` at audit time.

## Stage 2 limit

Implement only BF-034 (FakeSMK / F03 slot-probe fail-closed host negatives), on a new stage-2 branch from the pushed tip of `cursor/f15-f03-slotprobe-stage1-237f`. Do not take other deferred items in that same change.
