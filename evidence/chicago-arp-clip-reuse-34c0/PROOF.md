# Chicago arp → clip-mode reuse proof

- Branch: `cursor/chicago-arp-clip-reuse-34c0`
- Base: `cursor/stream4-combined-schema-53a1` @ `e280c753`
- Host: `X0X_TEST_JOBS=8` → `chicago-arp`, `smk-clip-keys`, `bass303-amp-adsr`, `bass303-wave-gnaw` — all ok (see `host_proof.log`)
- Change: `chi_arp_pick` / speed ticks are thin adapters onto `ck_arp_pick` / `CK_RATE_T`; live runtime + Chicago pots unchanged

## Selectors

| Selector | Result |
| --- | --- |
| `chicago-arp` | **ok** |
| `smk-clip-keys` | **ok** |
| `stage-handoff` | **ok** |
| `host-build` | **not-run** — Mac SDK `rusage_info_v4` (known host env gap; not a regression of this diff) |

Plan JSON: `local_proof_plan.json` (kind implement). GitHub Actions is not proof.
