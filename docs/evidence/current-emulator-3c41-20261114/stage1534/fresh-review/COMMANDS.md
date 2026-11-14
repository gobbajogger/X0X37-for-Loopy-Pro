# Executed commands and exits

All paths are relative to the workspace root. Commands below actually ran; every listed Node exit was 0. Node used v24.22.1. See each command log and JSON receipt for output.

| Command | Exit | Emulated ms |
|---|---|---|
| `node stage1534/fresh-review/boot-current.cjs elite` | 0 | 2502 |
| `node stage1534/fresh-review/exercise-current.cjs elite stage1534/fresh-review/continuity-running-plan.json continuity` | 0 | 10933 |
| `node stage1534/fresh-review/exercise-current.cjs elite stage1534/fresh-review/load-plan.json load` | 0 | 11919 |
| `node stage1534/fresh-review/exercise-current.cjs elite stage1534/fresh-review/musical-plan.json musical` | 0 | 17072 |
| `node stage1534/fresh-review/exercise-current.cjs elite stage1534/fresh-review/musical-qualified-plan.json musical2` | 0 | 17107 |
| `node stage1534/fresh-review/exercise-current.cjs elite stage1534/fresh-review/navigation-plan.json navigation` | 0 | 10579 |
| `node stage1534/fresh-review/exercise-current.cjs elite stage1534/fresh-review/navigation-qualified-plan.json navigation2` | 0 | 10759 |
| `node stage1534/fresh-review/exercise-current.cjs elite stage1534/fresh-review/settings-held-plan.json settings-held` | 0 | 16202 |
| `node stage1534/fresh-review/boot-current.cjs pro` | 0 | 2502 |
| `node stage1534/fresh-review/exercise-current.cjs pro stage1534/fresh-review/continuity-running-plan.json continuity` | 0 | 10933 |
| `node stage1534/fresh-review/exercise-current.cjs pro stage1534/fresh-review/load-plan.json load` | 0 | 11905 |
| `node stage1534/fresh-review/exercise-current.cjs pro stage1534/fresh-review/musical-plan.json musical` | 0 | 17081 |
| `node stage1534/fresh-review/exercise-current.cjs pro stage1534/fresh-review/musical-qualified-plan.json musical2` | 0 | 17107 |
| `node stage1534/fresh-review/exercise-current.cjs pro stage1534/fresh-review/navigation-qualified-plan.json navigation2` | 0 | 10770 |
| `node stage1534/fresh-review/exercise-current.cjs pro stage1534/fresh-review/settings-held-plan.json settings-held` | 0 | 16201 |

Total emulated scope including retained calibration attempts: 183572 ms.

Other successful local steps: `sha256sum` and byte-count verification of all five assigned inputs; scoped SHA-256 rehash against the authentic 3c41 source receipt; Node WebAssembly module validation/export inspection; `python stage1534/fresh-review/render-lcd.py` for raw RGB565-to-PNG conversion and contact sheets; viewer inspection of actual and extracted proposal PNGs. No firmware/host build or historical suite was run.

Initial navigation and musical plans were retained as stimulus-calibration evidence, not silently promoted to the intended targets. Navigation2 still contains an ignored `count` field on a PATCH tap; the harness does not read that field. The Settings-held plan, not that filename, establishes K3 editing. Musical2 uses PARA to choose FILTER 1 while a step is held; K2 at that point changes chance, not page.
