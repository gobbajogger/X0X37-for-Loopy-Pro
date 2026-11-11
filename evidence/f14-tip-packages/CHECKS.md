# F14 tip package validate (DEV CHECK) — BF-013 on current nightly

BF-013 is **already on firmware nightly** via historical [#74](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/74). Safe Merge [#325](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/325) was evidence-only. This record validates development packages on the **current tip**, not the diverged Stage-4 tip `656d70e9` from [#322](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/322).

| Field | Value |
| --- | --- |
| Worker | `bc-75dd783d-5755-591c-9f4d-f6575b5029b9` |
| Branch | `cursor/f14-tip-packages-237f` |
| Package source SHA (DEV CHECKED) | `b48cd8002a6d4d509010e04b8a8f1cfcbbdee64a` |
| `origin/nightly` at first validate | `b48cd800…` (post-#325) |
| `origin/nightly` re-fetch later | `077736829a7803a2dcec0b2a07242b20ae253386` — **BUILD_FAILED** (see below) |
| Content tree (`dev_validate.py::tree`) | `4eef581e267c582897df5910196671be2a04ac24a1d0a11acd6086fe583f3e60` |
| Git tree | `16b3caf29fc3548c290ce97afaed9f44214c1ab5` |
| Firmware tree | `1924e2cff82c4f1737fbe7a8188af1a3b229ae01` |
| `fxbus.c` blob | `032d289580d33df6a2d7cfe0e8a3d9b6fa5cbd2d` (historical #74; gates `bpm >= 20.0f`) |
| `stereo_test.c` blob | `c8bc1092319f38a46d4f8d41ab5b2a4ef560ff39` |
| Plan base | `a40e978cc55a2435bc131425148735bd7fcb11df` (#325 evidence parent) |
| Config | `X0X_DEV_PADCAL=1`, `BOARD=smk37`, controller built (`CTL_NO_CACHE=1`, `CTL_NATIVE=1`) |
| Host | `X0X_TESTS=stereo X0X_TEST_JOBS=8 X0X_DEV_PADCAL=1` PASS |
| Build path | Linux x86-64 native (`JIELI` clang 4.0.1); no `--worker-image` |
| Supersedes | #322 packages from diverged tip `656d70e9` (`34c381e1…` / `7498e646…`) — **do not publish those** |

No real MIDI opened. No device written. No flash. No production `main` / Pages. No Arp/Fx steal.

## Development packages

| Model | Result | Package | Package SHA-256 | Controller SHA-256 | code_free | ram_free | pool_free |
| --- | --- | --- | --- | --- | ---: | ---: | ---: |
| elite | **DEV CHECKED** | `x0x-smk37-elite.fwsc` (651396 B, SMK-37 Elite_900) | `4dc49bad6e8b555aefaf3a70f0ccb1579638d2b0e35c30a5f5fee043124f7376` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 2756 | 244 | 8308 |
| pro | **DEV CHECKED** | `x0x-smk37.fwsc` (651392 B, SMK-37 Pro_900) | `3f3b5c00148347d2dec8fdaf5933e606cd1755c6c55f80f550d5ef915f5a2980` | `956e3e214658aeaa06a8c122809399ef3e430c7192291f4d918b7b1181ec2d0a` | 2756 | 244 | 8308 |

Records: `elite/record.json`, `pro/record.json`.
Companion logs (learn from #319): `check-0.log`, `check-1.log`, `build.log`, `controller.log` beside each `record.json` with digests matching the records.
Pool floor ≥8192 **PASS** (8308). Controller unchanged (`956e3e21…`).

## Plan checks (host; not a package pass by themselves)

| # | Check | Exit | Result |
| --- | --- | ---: | --- |
| 0 | tip BF-013 blobs + `bpm >= 20.0f` / `BF-013` markers | 0 | PASS |
| 1 | `X0X_TESTS=stereo X0X_DEV_PADCAL=1 X0X_TEST_JOBS=8` | 0 | PASS — `ok stereo` |

## Verify

| Command | Exit | Result |
| --- | --- | --- |
| `dev_validate.py verify` elite | 0 | DEV CHECKED |
| `dev_validate.py verify` pro | 0 | DEV CHECKED |
| `require_gate_pass.sh … --dev-record` elite | 0 | **DEV OK** |
| `require_gate_pass.sh … --dev-record` pro | 0 | **DEV OK** |

## Attested package bytes (Stage 6 Mac fetch)

Prerelease tag `f14-tip-b48cd800-dev` (target `b48cd800…`): https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/tag/f14-tip-b48cd800-dev

| Asset | Download | SHA-256 |
| --- | --- | --- |
| Elite | …/releases/download/f14-tip-b48cd800-dev/x0x-smk37-elite.fwsc | `4dc49bad…` (full digests in table above) |
| Pro | …/releases/download/f14-tip-b48cd800-dev/x0x-smk37.fwsc | `3f3b5c00…` |
| ctl | …/releases/download/f14-tip-b48cd800-dev/ctl.img | `956e3e21…` |

`gh release download` re-fetch sha256 matched records. Not Stage-7 Pages publish. Do not flash a local Mac rebuild. Do not use #322 `656d70e9` packages.

## Tip move rebuild attempt (`07773682`)

Re-fetched `origin/nightly` after land; tip advanced via #319/#303/#315/#324. BF-013 `fxbus.c` blob unchanged (`032d2895…`). Host stereo on `07773682` PASS. Elite/Pro `dev_validate.py` **REFUSED**: linker RAM overflow **284 B** (`.bss` / `.pool` overlap) from `firmware/src/app/ui.c` (MIX WAVE / #303) on top of tip that had only `ram_free` 244 B at `b48cd800`. Log: `tip-07773682-elite-build-FAILED.log`.

Disposition: keep **DEV CHECKED** packages for package source `b48cd800` (BF-013 tip after #325). Do not claim packages for `07773682`. Coordinator: Stage 6 may install `b48cd800` hashes; a tip-at-`07773682` package pass needs a Stage-2 RAM repair first (≥284 B), then re-validate.

## Overall

**DEV CHECKED** Elite + Pro for package source `b48cd800` (BF-013 present). Ready for Stage-6 local install of **these** hashes. Current `origin/nightly` `07773682` is **BUILD_FAILED** for SMK packages until RAM is freed.
