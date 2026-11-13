# BugBot QA: preset measure stage-0 image identity, head cd9a1948

**Overall: PASS**

FMB claim verified: with default `DNBOX_PRESET_MEASURE_STAGE` (unset; the default is 0), Elite and Pro device images at `cd9a1948` are **byte-identical** to the `94197fc8` build. Every compared artifact matches, including the known Elite and Pro ELF hashes.

- **Repo:** gobbajogger/X0X37-for-Loopy-Pro
- **Branch:** `lane/fmb-presets-format` (GitHub `ls-remote` head matches this SHA)
- **Head:** `cd9a19480c82e97dd3b19530eb6c76b71bec5a18`
- **Head tree:** `c0a063a8bb4d321e57f5b425440df40f0934857e`
- **Range:** four commits on `7173c8fed9ade6d91b6897960680bddce0a5bdd2`
  - `d51f7ade43039a49414b4290e5f021133f3c1e43` — stage switch (`DNBOX_PRESET_MEASURE_STAGE` 0..4, default 0)
  - `679b761fb01bcb0f7c38720d6affaf6bcebba6e2` — capacity plan (docs only)
  - `61abd7d86d8c0a4dabb9facabb1bb0d7fab1cca3` — LTO keep fix
  - `cd9a19480c82e97dd3b19530eb6c76b71bec5a18` — JieLi STAGE=4 measurement record (docs only)
- **Baseline images:** `/workspace/kept-026-94197fc8/{elite,pro}/` and the matching local V1.2.14 rebuild at `/workspace/build-1214/artifacts/{elite,pro}/`
- **Reviewer:** BugBot, 13 November 2026, America/Chicago
- **Method:** `git fetch`, fresh detached worktree, rules / hook / LTO inspection, then **one** Elite and **one** Pro device build at this head with default stage 0, using the `/workspace/build-1214` SDK V1.2.14 recipe. sha256 compared against the `94197fc8` artifacts. No `STAGE!=0` measurement was redone. No 94197fc8 rebuild was needed (all hashes already matched). No device writes, no flashing, no pushes, no GitHub writes, no product-repo source edits. `/workspace/kept-026-94197fc8` was not overwritten.

This report path: `/workspace/audit-presets/qa-cd9a1948-bugbot.md`.

---

## 1. Checkout — **PASS**

```
git -C /workspace/x0x fetch origin lane/fmb-presets-format --no-tags
git -C /workspace/x0x worktree add --detach /workspace/qa-runs/cd9a1948 \
    cd9a19480c82e97dd3b19530eb6c76b71bec5a18
```

Verified in that worktree:

- `git rev-parse HEAD` = `cd9a19480c82e97dd3b19530eb6c76b71bec5a18`
- `git rev-parse HEAD^{tree}` = `c0a063a8bb4d321e57f5b425440df40f0934857e`

GitHub `ls-remote` for `refs/heads/lane/fmb-presets-format` is the same SHA. Logs: `/workspace/qa-runs/cd9a1948/logs/`.

---

## 2. Rules checks — **PASS**

**Changed paths** (`git diff --name-status 7173c8fe..cd9a1948`), and only these:

| Status | Path | Commits |
| --- | --- | --- |
| M | `docs/plans/2026-11-13-preset-format-v1.md` | `679b761f`, `cd9a1948` (docs only) |
| M | `firmware/src/app/main_fm1.c` | `d51f7ade`, `61abd7d8` |
| M | `firmware/src/app/preset_record.h` | `d51f7ade` (default `#define` 0) |
| M | `firmware/src/x0x.c` | `d51f7ade` (gated `#include` of the codec) |

Stat: 4 files, +272 / −5. No other paths.

**Not touched:** `.github/`, any workflow, anything under a `main` or `nightly` path, `tools/build.py`, `tools/package_nightly.py`, `firmware/src/app/factory_banks.c`, `firmware/src/app/project.c`, factory assets. `tools/build.py` and `tools/package_nightly.py` are **byte-identical** to the `94197fc8` tree (`cmp` against `/workspace/build-1214/base-94197fc8`). The branch recipe does not forward `DNBOX_PRESET_MEASURE_STAGE` (FMB's STAGE=4 measurement used a local copy of `build.py` under `/workspace/build-1214/presets-measure/`, which is not on the branch).

**Authors.** All four commits: author and committer are `gobbajogger <255244738+gobbajogger@users.noreply.github.com>`. No other author.

---

## 3. Device-build hook check — **PASS**, with a FLAG

The runtime measurement hook lives only in `firmware/src/app/main_fm1.c`:

- `preset_measure_touch` at line 2598, wrapped in `#if DNBOX_PRESET_MEASURE_STAGE` (opens at 2591, closes before `fm1_cstart`).
- One call from `fm1_cstart` at line 2674, also behind `#if DNBOX_PRESET_MEASURE_STAGE` (2673). The comment says it is once at init, never on the audio path. It is not in `ui.c`, not in any audio callback, and not in the `for (;;)` loop.

**FLAG (not a FAIL):** `main_fm1.c` is **not** the only device-path reference. `firmware/src/x0x.c` lines 214–221, also behind `#if DNBOX_PRESET_MEASURE_STAGE`, `#include "app/preset_record.c"` (and sets `PRESET_RECORD_NO_STRINGS`). That is the unity-build include that can link the codec when the stage is non-zero. FMB's own plan (section 14) already documents both sites.

`tools/build.py` has **no** `preset_record` or `DNBOX_PRESET_MEASURE_STAGE` reference (and is identical to `94197fc8`). Grep of the `7173c8fe..cd9a1948` diff and of the device unity list / `tools/build.py` found no other device-path site.

At the default (stage 0, or an unset define) both `#if DNBOX_PRESET_MEASURE_STAGE` guards are false (an undefined macro in `#if` is 0). The codec is not included, the hook and its call are compiled out, and `preset_record.h` (which now defaults the macro to 0) is not reached from either site. The byte-identical images in section 6 are the measured proof.

---

## 4. LTO keep fix (`61abd7d8`) — **PASS**, cannot affect the default image

The entire `61abd7d8` hunk is inside the existing `#if DNBOX_PRESET_MEASURE_STAGE` block in `main_fm1.c` (the block introduced by `d51f7ade`). That commit only:

- marks `preset_measure_touch` `__attribute__((used, noinline))`,
- changes `keep[]` from `volatile const void *` to `void *volatile`,
- drops the direct dummy calls and keeps a `void *volatile sink = keep[0]` so LTO / `--gc-sections` cannot drop the selected stage.

None of that is compiled when the stage is 0 / unset. The commit message itself says "Stage 0 is unchanged." Combined with the section 3 guards and the section 6 hash match, this fix cannot change the default image.

---

## 5. Exact build commands

Setup reused from `/workspace/build-1214/REPORT.txt`:

- SDK: `/workspace/build-1214/sdk-v1.2.14` (V1.2.14)
- Toolchain: `/home/box/.jieli/toolchain`
- `X0X_DEV_PADCAL=0`, `BOARD=smk37`, `X0X_JOBS=4`
- `DNBOX_PRESET_MEASURE_STAGE` was **unset** (not passed; no extra flags). Confirmed in both build logs as `<unset>`.
- Packaging identity matches the `94197fc8` baseline: `--nightly-version 0.0.26 --git-sha 94197fc839b00aff47b98ca02f3c6396e4a6eabf`. This is required for a fair identity test: `X0X_GIT_SHA` is compiled into `ui.c` (the short SHA display string). FMB's plan says a default stage-0 image "stays byte-identical to a base built with the same `--git-sha`." Source HEAD in the worktree was still `cd9a1948` (logged).

Source tree: `/workspace/qa-runs/cd9a1948` (not `/workspace/build-1214/base-94197fc8`, and not the presets-measure tree). Artifacts stayed under the worktree and `/workspace/qa-runs/cd9a1948/artifacts/`. `/workspace/kept-026-94197fc8` and `/workspace/build-1214/artifacts/{elite,pro}/` were not written.

**Elite** (13 November 2026, 06:24:03–06:24:11 AM America/Chicago; box 22:24:03–22:24:11 +10), exit 0. One build only. Log: `/workspace/qa-runs/cd9a1948/logs/elite-build.log`. Command script: `logs/elite-command.sh`.

```
unset GIT_DIR GIT_OPTIONAL_LOCKS
unset DNBOX_PRESET_MEASURE_STAGE || true
cd /workspace/qa-runs/cd9a1948
export JIELI_TOOLCHAIN=/home/box/.jieli/toolchain
export AC79_SDK=/workspace/build-1214/sdk-v1.2.14
export X0X_DEV_PADCAL=0 BOARD=smk37 MODEL=elite X0X_JOBS=4
python3 -u tools/build.py --board smk37 --model elite \
  --sdk /workspace/build-1214/sdk-v1.2.14 \
  --nightly-version 0.0.26 \
  --git-sha 94197fc839b00aff47b98ca02f3c6396e4a6eabf
```

Build log: app 544,232 B of 552,672 B, flash free **8,440 B**, RAM `.data+.bss` 95,536 B of 98,304, pool 335,724 B of 344,064 (pool free 8,340 B), identity `SMK-37 Elite_900`. Same figures as the `94197fc8` Elite log.

**Pro** (06:24:16–06:24:23 AM America/Chicago; box 22:24:16–22:24:23 +10), exit 0. One build only. Log: `/workspace/qa-runs/cd9a1948/logs/pro-build.log`. Command script: `logs/pro-command.sh`. Same env except `MODEL=pro` and `--model pro`.

Build log: app 544,224 B, flash free **8,448 B**, same RAM/pool as Elite, identity `SMK-37 Pro_900`. Same figures as the `94197fc8` Pro log. No `.ufw` is produced by this pipeline (`.fwsc` only).

**Bundled package** (compose only; not a second firmware compile), same controller as the baseline (`/workspace/build-1214/artifacts/ctl-a334208b.img`, sha256 `a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984`, 61,552 B). Output under `/workspace/qa-runs/cd9a1948/artifacts/` only. Both compose and `--check` exit 0. Logs: `logs/pkg-elite.log`, `logs/pkg-pro.log`, `logs/pkg-check.log`.

```
PYTHONPATH=tools python3 -B tools/package_nightly.py \
  /workspace/qa-runs/cd9a1948/build/x0x-smk37-elite.fwsc \
  /workspace/build-1214/artifacts/ctl-a334208b.img \
  --model elite \
  --controller-sha256 a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984 \
  --controller-size 61552 \
  --firmware-version 0.0.26 \
  -o /workspace/qa-runs/cd9a1948/artifacts/elite/DnBOX-0.0.26-SMK37-Elite-94197fc8.fwsc
```

Pro: same with the Pro raw `build/x0x-smk37.fwsc`, `--model pro`, and the Pro `-o` path. Then `--check` on each bundled file with the same controller sha/size and version (exit 0).

---

## 6. sha256 pairs — all **MATCH**

A 94197fc8 rebuild in this setup was **not required**. Every artifact already matched both `/workspace/kept-026-94197fc8` (where a sibling exists) and the prior local V1.2.14 rebuild at `/workspace/build-1214/artifacts`. Those two baselines already agreed with each other (and with the known ELF hashes in the brief). Hash list: `/workspace/qa-runs/cd9a1948/logs/sha256-pairs.txt`.

`/workspace/kept-026-94197fc8` was left intact (Elite ELF mtime 13 Nov 05:02 +10, Pro ELF the same, bundled fwsc mtime 20:13 +10; hashes unchanged after this QA).

### Elite

| Artifact | Bytes | sha256 (cd9a1948 = 94197fc8) | Result |
| --- | --- | --- | --- |
| `x0x.elf` | 628,872 | `720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea` | MATCH (kept-026 and build-1214; known hash in the brief) |
| `x0x.bin` (app) | 544,232 | `c6d651a12a71ea4c8c5280cdeb2b854ebd1400ba57b342a641f236626d70c9ef` | MATCH (build-1214) |
| raw `.fwsc` (`x0x-smk37-elite.fwsc`, from `build.py`) | 651,732 | `af1d1c8ac0c66696a89b3b635fa1ba54872a6c9915c93708f68dbd74172ca97c` | MATCH (build-1214 raw) |
| bundled `.fwsc` (`DnBOX-0.0.26-SMK37-Elite-94197fc8.fwsc`) | 651,732 | `95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6` | MATCH (kept-026 `*-hwtest.fwsc` and build-1214 bundled) |
| `loader/ota.bin` | 7,616 | `0413ab513ab3bbe9661ec9479fc77b237801b2d2067685c02c78ab5048db2e03` | MATCH (build-1214) |
| `loader/loader.elf` | 11,536 | `087db868579219b01369ed65e3ec1cb51f4dfa2b36ee540d214f261c2348f27b` | MATCH (build-1214) |

cd9a1948 Elite paths: `build/smk37-elite/x0x.elf`, `build/smk37-elite/x0x.bin`, `build/x0x-smk37-elite.fwsc`, `artifacts/elite/DnBOX-0.0.26-SMK37-Elite-94197fc8.fwsc`.

### Pro

| Artifact | Bytes | sha256 (cd9a1948 = 94197fc8) | Result |
| --- | --- | --- | --- |
| `x0x.elf` | 628,864 | `7e67c3b33fe2e690e5f913b5d048727a715c70daca06ddd326a10e37eacb9535` | MATCH (kept-026 and build-1214; known hash in the brief) |
| `x0x.bin` (app) | 544,224 | `b52b29e667384cd796d29e47360e879f5b447702c7e2413828310b28bb7d2dbd` | MATCH (build-1214) |
| raw `.fwsc` (`x0x-smk37.fwsc`, from `build.py`) | 651,730 | `1fd4809ef080d5bf177e96fb73914f5c0deb0d9636503dbc0f7532c8f1cefc61` | MATCH (build-1214 raw) |
| bundled `.fwsc` (`DnBOX-0.0.26-SMK37-Pro-94197fc8.fwsc`) | 651,730 | `ce67d560eebd03cb8b9087f7723b1e0b71fb7cca256184cdfdd55a8c27c708b8` | MATCH (kept-026 `*-hwtest.fwsc` and build-1214 bundled) |
| `loader/ota.bin` | 7,614 | `d5874d17f139fc231275917a041dd2a2c98434ce80dcae26733fb55aae605bd5` | MATCH (build-1214) |
| `loader/loader.elf` | 11,532 | `083e69116344fe72bb00ef331b75bccf3f1b75252e6f3726e83f0a00df3d6e28` | MATCH (build-1214) |

cd9a1948 Pro paths: `build/smk37/x0x.elf`, `build/smk37/x0x.bin`, `build/x0x-smk37.fwsc`, `artifacts/pro/DnBOX-0.0.26-SMK37-Pro-94197fc8.fwsc`.

No section-level `objdump` / `cmp` was needed: the whole files, including ELF, are identical. The difference is not from the code change and not from the environment — there is no difference.

---

## 7. Scope notes

- STAGE!=0 measurements were **not** redone. The STAGE=4 record in the plan (`cd9a1948`) and the measurement-only images under `/workspace/build-1214/artifacts/presets-measure/` were not rebuilt or re-flashed. Those images remain measurement-only, not for install.
- Host tests were not re-run here (out of this assignment; already measured at `7173c8fe`).
- The two docs-only commits (`679b761f` capacity plan, `cd9a1948` measurement record) cannot affect the device image.
- Worktree status after the builds: untracked `logs/` and `artifacts/` only. No firmware or tools source was edited. `build/` is gitignored.

---

## 8. Verdict

**PASS.** Rules, LTO-fix isolation, and the FMB stage-0 identity claim all hold. The one FLAG is that `x0x.c` is a second (also gated) device-path site alongside the `main_fm1.c` hook; at default stage 0 both sites are compiled out, and the images are byte-identical to `94197fc8`.
