# Stage 2 checks for F08 (BF-004)

Branch: `cursor/f08-bf004-stage2-fb90`
Stage-2 owner: `bc-b8f70f77-86ea-5f4d-8f65-8b52329fee6e`
Base audit tip: `cdb68e5686234cedb5e894d61aba88b8892b8e3b` (`cursor/f08-bf004-stage1-fb90`)
Audited / validated source SHA: `6df0f81fbbe0adc3207f67d912d2fe0ab676be04`
Content tree (pre-repair package baseline): `69ccb3d77a718d32191cf17a75bce6454dad807ef9bb76f6050e6532a0bff3ad`
Git tree (pre-repair): `4dc57f74141fd1f01728a0001025b6e76065bf19`
Audited firmware content tree (package bytes baseline): `b02108f768a1d6cea2bb78f534f224bbace3c9dfce9697e1dfd00fe69ddd5190`
Audited firmware reference SHA: `1325f5d34340ea877e65ab86f3663f5dea052c0d`
Repair: **BF-004 only** — ownership-gated 303 / BREAK Note Off in `firmware/src/app/engine.c` + host scenario `tests/scenarios/midiowner.x0x`.
Engine blob before: `555480d9c81ca8b088842bfa6bac93e85865d49a`. After: `ab6d9667e9cd40ae12cc8c8ece8d7bb12be29216`.
No production `main` / production Pages update. No device flash. GitHub Actions were not used as tests.
BF-001 / BF-002 / BF-003 / FP-001 not reopened. BF-005 / BF-006 / BF-007 not taken.

## Local proof (this head)

| Check | Result | Log |
| --- | --- | --- |
| `midiowner.x0x` on unfixed tip (`555480d9…`) | **FAIL** exit 1 — stale non-owner Note Off cleared 303A (expected 1 got 0) and BREAK (expected 256 got 0); reverse ownership and TRS-takeover stale Off also FAIL | `evidence/f08-stage2/midiowner-red.log` |
| `midiowner.x0x` after repair | **PASS** exit 0 — stale Off ignored; owner Off / different-note / same-owner / TRS release green | `evidence/f08-stage2/midiowner-green.log` |
| `usbnotes.x0x` after repair | **PASS** exit 0 | `evidence/f08-stage2/usbnotes.log` |
| `blenotes.x0x` after repair | **PASS** exit 0 | `evidence/f08-stage2/blenotes.log` |

Host built with `sh host/build_host.sh` (development tree; `X0X_DEV_PADCAL=1` for any later firmware package rebuild in Stage 4). Pillow installed via `tools/agent_env_install.sh` for host PNG path.

## Repair summary

- Gate 303 Note Off on `src_held[src]` ownership when `src < 2` (USB/Bluetooth); TRS (`src >= 2`) still releases.
- Gate BREAK slice Note Off the same way before `breaks_live` / clearing ownership bits.
- Note On ownership transfer and link-drop per-source release unchanged in intent.
- New scenario `tests/scenarios/midiowner.x0x` covers stale Off (303 + BREAK), owner Off, different-note, same-owner, reverse USB/BT takeover, and TRS behavior.

## Deferred

See `evidence/f08-stage2/DEFERRED.md`.

## Overall

**IMPLEMENT CHECKED.** Ready for Stage 3.
