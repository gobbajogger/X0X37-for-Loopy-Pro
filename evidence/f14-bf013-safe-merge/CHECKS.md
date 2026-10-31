# F14 BF-013 Safe Merge onto current nightly

Agent: `bc-31734ec9-0680-51d9-bb9d-40745c5ad7d9`
Branch: `cursor/f14-bf013-safe-merge-237f`
Base: `origin/nightly` @ `811d7dd2fd13d4ca13ac3d899fb202758ae997a7`
Pattern: same as BF-012 Safe Merge [#308](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/308) — transplant repair onto current nightly; do not rewind nightly to old cycle tip `656d70e9`.
Do not resume Stage 5 `bc-b08d7670-f066-5496-8972-62767b638eb4` as Stage 7. No force-push. No Pages.

## Disposition

**ALREADY_ON_NIGHTLY** — no firmware delta to transplant.

| Field | Value |
| --- | --- |
| BF-013 repair commit | `13b831e8` (`>= 20.0f` gates) |
| Expected `stereo_test.c` blob | `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39` — **exact match on tip** |
| Expected `fxbus.c` repair-only blob | `49a89483e1c777a46758ed370973afb85f15274d` |
| Tip `fxbus.c` blob | `032d289580d33df6a2d7cfe0e8a3d9b6fa5cbd2d` |
| Tip vs repair-only | Same `bpm >= 20.0f` gates; tip also uses `x0x_pot_exp_value()` (unrelated flash win). **Keep tip blob** — do not rewind pot packing. |
| Prior land | PR [#74](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/74) / `af67d575` + `31dd6388` are ancestors of `811d7dd2` |
| Cherry-pick `13b831e8` | empty (content already present) |
| Divergent F14 tip | `656d70e9` — Stage 5 non-FF refusal vs `811d7dd2` (merge-base `de11b6d7`) recorded on [#323](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/323) |

No reopen of BF-001–008 / BF-012 / FP-001. No Arp/Fx steal.

## Host proof (this tip)

Config: `X0X_TESTS=stereo X0X_TEST_JOBS=8 X0X_DEV_PADCAL=1`

| Check | Exit | Result | Log |
| --- | ---: | --- | --- |
| Red witness (`>` gates only) | 1 | FAIL_EXPECTED — four 20 BPM assertions | `host-stereo-red.log`, `stereo-red-detail.log` |
| Green (tip restored) | 0 | PASS — `ok stereo` | `host-stereo-green.log`, `stereo-green-detail.log` |
| Green reconfirm | 0 | PASS — `ok stereo` | `host-stereo-green-reconfirm.log` |

## Packages

**No rebuild for land** — firmware bytes for BF-013 already on tip; no Safe Merge source change. F14 Stage 4 packages from divergent tip `656d70e9` (Elite `af5cd709…`, Pro `eba70777…`) must **not** be published as the current nightly candidate. Stage 6/7 must use packages built from tip after this land (or current tip `811d7dd2` if evidence-only tip unchanged). Companion logs (`check-0.log`, `controller.log`, `build.log`) required on any future DEV CHECK rebuild (learn from #319).

## Nightly land

Evidence-only commit(s) on this branch; firmware tree unchanged vs `811d7dd2`. Fast-forward `nightly` when remote candidate matches. No force-push. No Pages publish from this job.

## Stage 6/7

Use the **post-land nightly tip** (this Safe Merge tip if evidence FFs; else `811d7dd2`). Do **not** use divergent F14 tip `656d70e9` or resume Stage 5 agent as Stage 7.
