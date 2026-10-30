# Stage 7 — final WBL1 nightly publish

| Field | Value |
| --- | --- |
| Published **source** SHA | `0e77d923dfee44a9b64a98c121fac55200b91f20` |
| Includes | #606 WBL1, #611 `37a768f3` (−220 B), #612 MIDI RT MQ |
| `origin/nightly` at publish time | advanced past `0e77d923` (see final-build doc) |
| Catalog `latest` | `nightly-0e77d923-dev` |
| Content tree (DEV CHECKED) | `68055d98e7139d114c31b14bfeef6cb9a532cf9456676048a427e19f22786bde` |
| Artifacts host | `artifacts/nightly-dev` @ `5f48ec6541ca670739574ce3dae217b4b39d223e` |
| Pages `gh-pages` | `730835e` (nightly repo) |
| Label | `DEV / HARDWARE TEST — DEVICE NOT TESTED`; `hardware_tested` false |

## Packages (bundled page bytes)

| Model | Blank sha256 | Page sha256 | pool_free |
| --- | --- | --- | ---: |
| Elite | `b1be499d…` | `afea412d…` | 21092 |
| Pro | `e986db96…` | `d05bf2af…` | 21092 |
| ctl | `956e3e21…` | embedded | — |

## PR sweep

No BETTER open PRs vs publish SHA; #212/#21/#1 park unchanged.

## Stage 6

Mac flash **pending** — cloud cannot reach `X0X FM-1`; coordinator must run `internal/mac-final-flash-0e77d923-run.sh` on worker `9a5ea019-7565-5f4c-be4d-f33137cc0aa0`.

## Overall

**PUBLISHED LIVE** — supersedes `nightly-b48cd800-dev` / stale `b48cd800`.
