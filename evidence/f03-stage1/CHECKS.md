# Stage 1 checks for F03 audit

Branch: `cursor/f03-stage1-fb90`
Stage-1 owner: `bc-2e0f6b6e-50b2-506d-92f4-e19a81d6cfac`
Validated source SHA: `4ee8f37714b0892f2c70d157fda9328da393e987`
Content tree: `7ad3a50da8abac890215dae3c28197a2fa90041b8615e828b6b3c6e18060fb83`
Git tree: `c63af7972f3ca6e5d4e8ac3aac5e70abc8178265`

## Live nightly page vs assignment

| Field | Expected | Observed |
| --- | --- | --- |
| Installer URL | nightly install page | HTTP 200 |
| `latest` | `nightly-4ee8f377-dev` | MATCH (live HTML) |
| Label | `DEV / HARDWARE TEST — DEVICE NOT TESTED` | MATCH |
| `hardware_tested` | false | MATCH |
| Elite bundled | `8ba23a89…2278` / 651396 | re-fetch MATCH |
| Pro bundled | `d9093211…fecb` / 651392 | re-fetch MATCH |
| Elite blank | `d3b4cac3…5f2b` / 651396 | re-fetch MATCH |
| Pro blank | `0b09605f…76b9` / 651392 | re-fetch MATCH |
| Controller (bundled) | `956e3e21…2d0a` | catalog MATCH |
| Host | `artifacts/nightly-dev` @ `f917b636…` | MATCH |
| Pages tip | `364c5509…` | Stage-7 record (not re-pushed) |

## Source tip vs defect order

| Item | Result |
| --- | --- |
| F02 in `4ee8f377` | Present (closed); not selected |
| F03 in `4ee8f377` | Still open (`slot_probe` → `None` → `plan_slots` skip) |
| Open FAIL checks | None (Actions ignored) |
| Selected | **F03** |

## Production / nightly refs

| Ref | Action |
| --- | --- |
| Production `main` | unchanged |
| Production GitHub Pages | unchanged |
| Firmware `nightly` | not force-pushed; audit uses `4ee8f377`, not diverged tip |
| Device flash | not performed |

## Overall

**AUDIT RECORDED.** Defect F03 selected. Ready for Stage 2 after owner-visible successor is on this branch tip.
