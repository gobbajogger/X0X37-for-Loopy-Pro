# Post-tip Stage 1 checks (b48cd800)

| Check | Result | Evidence |
| --- | --- | --- |
| Live `versions.json` latest | `nightly-b48cd800-dev` | `live-verify.log` |
| Elite/Pro/ctl HTTP re-fetch | **MATCH** Stage-7 digests | `live-verify.log` |
| Pages tip `a338960c…` | **MATCH** | `live-verify.log` |
| `install_f03_slot_probe_test.py` | **FAIL** exit 1 (BF-034) | `bf034-f03-slotprobe-witness.log` |
| BF-034 ownership | **owned** by `bc-c8e90cb7…` — no second Stage 2 | `AUDIT.md` |
| Tip `07773682` RAM | **CLEARED** by #329 reméasure | `tip-ram-remeasure-note.md` |
| Other tip FAIL | **none** selectable | `AUDIT.md` |
| Production edits | **none** | this audit |

Overall: **LIVE_PAGE_MATCH**; **defect for new Stage 2: no** (BF-034 owned elsewhere).
