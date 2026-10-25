# Chicago SEND FX — host proof

| Item | Value |
| --- | --- |
| Head | `1a556b2ab91a04a939247c612bac1c6416b4f6e4` |
| Base | `a692adfd` (stream5 after #281) |
| Jobs | `X0X_TEST_JOBS=8` |

## Selectors

| Selector | Result |
| --- | --- |
| `chicago-fx-bridge` | **ok** |
| `project-stage` | **ok** |
| `bass303-amp-adsr` | **ok** |
| `sequencer` | **ok** |
| `stage-handoff` | **ok** |
| `host-build` | **not-run** — host SDK `rusage_info_v4` / `libproc.h` compile break on this Mac (reproduced on unrelated tip `chicago-arp-ab91`; not caused by this diff) |

Planner JSON: `local_proof_plan.json` (kind implement; app → host-build). Extra selectors run for schema + new bridge test.
