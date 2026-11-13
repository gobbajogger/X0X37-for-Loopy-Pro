# QA de4c13c4 — opt-flash-savings-026 (replaces failed 5849cb11)

**Overall: FAIL**

Independent BugBot pass of `/workspace/x0x37.git` branch `lane/opt-flash-savings-026`, head `de4c13c4f395973e5dc4e4879e41e849ad632580` (tree `29ad34388d75231a2d3a4e8cf5cf4a739811114c`), base `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. Fresh worktrees: `/workspace/qa-runs/de4c13c4`, `/workspace/qa-runs/94197fc8`. Harness: `/workspace/qa-runs/5849cb11-harness/de4c13c4/`. Logs: `/workspace/qa-runs/de4c13c4/logs/`. Times below are America/Chicago (UTC−6); box stamps were UTC+10.

Blocking: host suite on head introduces a **new** FAIL vs base — `optimization-equivalence` link error, multiple definition of `fx_beats` (commit `de4c13c4`). Bit-identity, not-slower, size/RAM, and rules that finished are PASS. `tools/quickcheck.sh` was **NOT_RUN**. Head `tests/run_tests.sh` was still running at report time; the new FAIL is already recorded.

`5849cb11` remains in history (old O(pot) pack, previous QA FAIL) and must not ship; `59282a84` is the O(1) replacement that was measured here.

---

## 1. Bit-identity — **PASS** (all mismatches = 0)

Harness + Python table compare vs 94197fc8 / gold / pinned private headers. Log: `5849cb11-harness/de4c13c4/out/identity-tables.txt`, `logs/harness-bench.log`, `5849cb11-harness/de4c13c4/out/elf-table-identity.txt`. ASan: `harness_asan` EXIT=0, EXP mismatches=0.

| Check | n | mismatches |
|---|---:|---:|
| EXP 19×128: old 94197fc8 decoder == new linear decoder == gold `pot_exp` | 2432 | 0 |
| Gold SHA `c8c351223b0c805e68f88c917c5ed0a8b4983277681e2a1b43251627ffbd3faf` (same as prior gold) | | |
| `original_pitch44100` header + ELF (base 2 copies, head 1) vs src SHA `77c39cdf…` | 128 / 512 B | 0 |
| `IMA_ROUND_CORR` 64 B (base 2 copies, head 1) + `IMA_STEP_T`/`IMA_IDX_T`; `ima_decode` body identical; 7×4 starts × 2048 nibbles | 64 + decode | 0 |
| `x0x_q15_ramp258` vs pinned `miami_decimator_wet`, `mod_linear`, `original_drive_table`, and `i*258` (i=0..126)+32767; ELF 256 B | 128 | 0 |
| Chorus `x0x_q15_ramp1024(p)` vs pinned `mod_wet[p]` (p=0..31 = p*1024, p≥32 = 32767) | 128 | 0 |
| `fx_beats` 13 IEEE bits: base fxbus == base `chi_bpmdly` == head fxbus == head DEFINE copy == ELF (base 2 copies, head 1); clamp div 0..255 | 13 / 256 | 0 |
| Four `*_private.h` blobs unchanged (`0abf5bc4`, `6aa5131b`, `695c2201`, `b199b9e7`) | 4 | 0 |

OOB pot=128/200: neither decoder clamps (callers clamp 0..127); values need not match. Documented only.

---

## 2. Not-slower — **PASS**

Host: `gcc -std=c99 -O2 -fno-fast-math -ffp-contract=off`, `CLOCK_THREAD_CPUTIME_ID`, `nice -n 19 taskset -c 3`, 11-run medians. Log: `logs/harness-bench.log` (00:18:26–00:19:56 +10 ≈ 08:18–08:19 AM Chicago). Compared vs **94197fc8 O(1) pack**, not vs 5849cb11.

- `x0x_pot_exp_value` pot 0..127: **0 / 128 slower**. Mean of per-pot medians **2.4952 → 1.8564 ns (−25.60%)**. Closest (still faster): pot 96, −0.0389 ns (0.9804×). pot 127: 2.6875 → 1.8236 ns (0.6785×). Sweep 19×128: 2.5833 → 1.9264 ns (0.7457×). (Optimizer cited −15.66% on a different same-run uniform; our independent numbers are faster, not slower.)
- Chorus wet formula vs table: 2.3124 → 2.2052 ns (0.9536×).
- `ima_decode` (identical formula): 3.2804 → 3.2584 ns.
- ramp258 shared vs priv load (identical data): 2.1810 → 2.1588 ns.
- `fx_beats` helper: first 11-run median 2.1742 → 2.1778 ns (**+0.0036 ns**, 1.0016×) on **identical** clamp+table code (only the array name differs). Run ranges fully overlap (old 2.1547–2.2669, new 2.1474–2.3031). Longer 21-run identical-code control (`logs/beats-noise-control.log`): median **−0.01235 ns** (0.99436×). Measurement noise, not a firmware regression. All actually-changed paths are faster.

Device JieLi `pi32v2/bin/objdump` (Elite=Pro for pot):

- `x0x_pot_exp_value`: base **26 raw insns / 72 B**; head **21 raw insns / 68 B**; **no loop** (64-bit load `r7_r6 = d[r0+0]`, 64-bit shift, `*0x15` / `*0x54`). Optimizer’s “22 vs 28 normalized” is a different count; raw is 21 vs 26. Disassembly: `5849cb11-harness/de4c13c4/out/dis-x0x_pot_exp_value-head-elite.txt` (and `-base-elite.txt`).
- `original_chicago_chorus_set` (COLD): 29 → 31 raw insns (table load replaced by `<<10` and `if (r3 <= 31)`); host setter is faster, as above. Process path still uses stored `pending_wet`.

---

## 3. Host suite and quickcheck — **FAIL** (new head failure); quickcheck **NOT_RUN**

Official refs: `NINEW9_DIR` / `EIGHTW8_DIR` / `SCHWUNG303` / `TB3PO_REF` under `/workspace/x0x37-refs/schwung/`. Pillow 12.3.0, ASan links. Commands: `logs/full-suite-94197fc8-command.sh`, `logs/full-suite-de4c13c4-command.sh`. `X0X_TEST_JOBS=8`, `X0X_TEST_TIME_SCALE=20`.

Default-absent pre-existing (confirmed **identical** on 94197fc8 and de4c13c4, accepted): `run_chicago_bpmdly.sh` exit 1, `bass303.h: No such file or directory` (`logs/preexist-bpmdly-*.log`); `drum909` without `NINEW9_DIR` exit 2, `9W9 sources not found` (`logs/preexist-drum909-*.log`). With official refs, base `drum909` and chicago tests **ok**.

**Base 94197fc8 — complete** (`logs/full-suite-94197fc8.log`): 08:21–08:49 AM Chicago (00:21:47–00:49:04 +10). **433 ok, 2 FAIL, 1 skip, 0 not-run, EXIT=1.**

- `FAIL engine-808-scratch-alias`: `fatal: invalid object name 'origin/build/0.0.22'` (box bare repo has no `origin`). Log copied: `logs/base-engine-808-scratch-alias.log`.
- `FAIL offline-sequencer-browser`: Playwright chromium missing under the test’s isolated HOME. `logs/base-offline-sequencer-browser.log`.
- `skip SMK-37 Pro package tests` (no in-tree `build/smk37` app/loader for that worktree).

These two FAILs are box-environment, already present at the base SHA.

**Head de4c13c4 — incomplete at report, already a new FAIL.** Started 08:49 AM Chicago (00:49:24 +10); still running at 09:00 AM Chicago (01:00 +10) with **158 ok, 2 FAIL** in `.results`.

- `FAIL engine-808-scratch-alias`: **same** `origin/build/0.0.22` as base.
- **NEW** `FAIL optimization-equivalence` — **ok on base**. After a3 fxbus −O0/−O2 equivalence already **PASS**, `fxbus_idle_identity_test.sh` fails at link:

```
multiple definition of `fx_beats'
collect2: error: ld returned 1 exit status
```

Log: `logs/head-optimization-equivalence.log` (copy of `build/host/optimization-equivalence.log`). Cause: `de4c13c4` made `fx_beats` a non-static global in `firmware/src/dsp/fxbus.c`; the idle-identity harness compiles two fxbus TUs. This is a **head-vs-base regression** in the host suite. Item 3 is **FAIL**. Remaining head tests not finished here. **NOT_RUN is not a pass.**

**`tools/quickcheck.sh`:** **NOT_RUN** (not started; low-usage steer). Isolated `X0X_QC_WORK` + `X0X_QC_BASE=94197fc8` was the intended recipe.

---

## 4. Elite+Pro size/RAM — **PASS** (−2852 B both models; RAM/pool unchanged)

`python3 -u tools/build.py --board smk37 --model {elite,pro} --sdk /workspace/build-1214/sdk-v1.2.14 --nightly-version 0.0.26 --git-sha 94197fc839b00aff47b98ca02f3c6396e4a6eabf`, `X0X_DEV_PADCAL=0`. Exact commands: `logs/{elite,pro}-command.sh`. Logs: `logs/{elite,pro}-build.log`. Artifacts: `/workspace/qa-runs/de4c13c4/artifacts/{elite,pro}/`. Same `--git-sha` as kept-026 / build-1214 base images (not rebuilt).

| | Elite | Pro |
|---|---:|---:|
| Base flash free | 8440 | 8448 |
| Head flash free | **11292** (=8440+2852) | **11300** (=8448+2852) |
| Base image | 544232 | 544224 |
| Head image | **541380** (=544232−2852) | **541372** (=544224−2852) |
| `.data` / `.bss` / combined | 588 @ `0x01c08000` / 94944 @ `0x01c08250` / **95536** (unchanged) |
| Pool used / `_bss_end` / `_pool_*` | **335724** / `01c1f530` / `01c20000`–`01c71f6c` (unchanged) |

`.text` shrinks by exactly 0xB24 = 2852. EXP payload: `x0x_exp_words` 8100 → 6388 B, rows 152, decoder 72 → 68 B. Pitch 2×512 → 1×512. IMA 2×64 → 1×64. Beats 2×52 → 1×52. Head ELF has one `x0x_q15_ramp258` 256 B; `mod_linear` / `miami_decimator_wet` / `mod_wet` absent (shared or formula).

Head SHA256: Elite ELF `f1d5ffc69290360bca4d4679fbdb5cb50c03f4a1bb15ad6f52e4376909fe5020`, bin `607d31e1e9cc95c418766f11bf46ae5641a5168b0aaa354570740aac4f2e6209`; Pro ELF `c3ee93991eb630dd330b3bcaa23182d134527a7db0ff239d659661f0b764bbd7`, bin `918170994b65eb432b6510b3720edbfd37b2accbf3c5b7aecf22d971ddaf1b08`. Optimizer published bins (`64b30afb…` / `2358b3ed…`) differ — expected if they baked a different `--git-sha`; sizes match. Base bins in build-1214 match known `c6d651a1…` / `b52b29e6…`. No flashing, no binaries posted. `kept-026` not overwritten.

---

## 5. Rules — **PASS** (GitHub presence: box-only, 404)

- All 8 commits `94197fc8..de4c13c4` (including leftover `5849cb11`) author **and** committer `gobbajogger <255244738+gobbajogger@users.noreply.github.com>`.
- Files only: `experiments/original_synth/chicago_drive.c`, `chicago_modulation.c`; `experiments/wobble/miami_decimator_adapter.c`; `firmware/src/app/chicago_bpmdly.h`; `firmware/src/dsp/{fxbus.c,ima_adpcm.h,original_chicago_backend.c,original_miami_backend.c}`; `tests/host/{breaks_reverse_seek_perf_test.c,chicago_au_render.c,chicago_bpmdly_test.c,chicago_fx2_inserts_test.c,ima_decode_perf_test.c,ima_delta_lut_exact_test.c}`; `tools/gen_drum_samples.py`. No `main` / `nightly` / `.github`. Branch exists only as `lane/opt-flash-savings-026` on the box bare repo (no remotes).
- No `*_private.h` in the diff; four private headers byte-identical (see §1).
- No feature removed: callers still go through shared tables / formula (`x0x_pot_exp_value`, `x0x_q15_ramp258` aliases for drive/miami/mod_linear, `x0x_q15_ramp1024` in chorus_set, `chi_bpmdly_beats` → `fx_beats`, `ima_decode`, Chicago `extern` + Miami alias for `original_pitch44100`).
- GitHub `gh api repos/gobbajogger/X0X37-for-Loopy-Pro/branches/lane/opt-flash-savings-026` → **404**. Expected box-only. Notes the all-work-to-GitHub rule; not a code FAIL.

---

## Verdict

**FAIL** on item 3: head `optimization-equivalence` is a new host-suite failure vs 94197fc8 (`multiple definition of fx_beats` in `fxbus_idle_identity_test.sh`), caused by making `fx_beats` a non-static global. Items 1, 2, 4, 5 that finished are PASS (−2852 / RAM / bit-exact / not-slower / authors). `quickcheck.sh` and the remainder of the head suite are **NOT_RUN** / incomplete and are not passes. Head suite process was left running (not killed). Fix is on the Optimizer lane: one `fx_beats` definition when two fxbus TUs are linked (host harness), without undoing the −52 B share.
