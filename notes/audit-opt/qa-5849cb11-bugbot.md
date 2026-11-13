# BugBot QA: flash-savings head 5849cb11

**Overall: FAIL** (owner not-slower rule; blocking)

`5849cb11` replaces an O(1) EXP pot lookup with an O(pot) IEEE-bit delta loop. Independent host microbenchmark and the Optimizer's own CPU-check log both show `x0x_pot_exp_value` is substantially slower (about 34× on a uniform 19×128 sweep; about 61–64× at pot=127). The function is on the cold UI/MIDI pot path, not the audio render path, but the owner rule is "any build slower than previous is a blocking failure," and the decoder itself is slower. `d99a2588` (shared pitch table) is not slower.

This head is expected to be replaced by an O(1) pack. Heavy work was stopped on parent request: `quickcheck.sh` was never started; the full host suite was started and then killed incomplete. Finished work is kept (rules, bit-identity harness, Elite+Pro default device builds, microbenchmark). The harness at `/workspace/qa-runs/5849cb11-harness` is reusable for the next head.

- **Repo (box):** `/workspace/x0x37.git`, branch `lane/opt-flash-savings-026`
- **GitHub:** branch and SHAs are **not** on `gobbajogger/X0X37-for-Loopy-Pro` (`gh api` 404; `ls-remote` has no `opt-flash` / `5849cb11` / `d99a2588`). Box-only. That is against the all-work-to-GitHub rule (noted; not the overall FAIL reason).
- **Base:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`
- **Head:** `5849cb114b20212c22d75b28839d021696fd4d68` (tree `cb85d094f7eb27bc70d37323c82bde8125da8521`), verified in a fresh detached worktree `/workspace/qa-runs/5849cb11`
- **Commits:** `d99a25881491fe7363477022724ca2ee0c9fc212` (share pitch table), then `5849cb11` (EXP IEEE-bit deltas)
- **Reviewer:** BugBot, 13 November 2026, America/Chicago
- **Report:** `/workspace/audit-opt/qa-5849cb11-bugbot.md`

No device writes, no flashing, no pushes, no GitHub writes, no product-repo source edits. `/workspace/kept-026-94197fc8` was not overwritten.

---

## 1. Rules — **PASS**

`git diff --name-status 94197fc8..5849cb11` is only:

| Status | Path | Commit |
| --- | --- | --- |
| M | `firmware/src/dsp/original_chicago_backend.c` | `d99a2588` |
| M | `firmware/src/dsp/original_miami_backend.c` | `d99a2588` |
| M | `tools/gen_drum_samples.py` | `5849cb11` |

Both commits: author and committer `gobbajogger <255244738+gobbajogger@users.noreply.github.com>`. No `main`, no `nightly`, no `.github`.

`experiments/original_synth/motion_tables_private.h` is **byte-identical** to base (blob `0abf5bc47632aa38b58e52dfc91d58c0e5ee0ca5`, SHA256 `92c856c0fe1f7811ec938bbffd4f0cdcbe8b86630eaf537f63ab8ff3d32cb564`). The file was read only as a table source; `*_private.h` is not treated as a security finding.

---

## 2. Bit-identity — **PASS** (independent harness)

Harness: `/workspace/qa-runs/5849cb11-harness` (outside the repo; kept for the next head). It compiles the **old** table source from the 94197fc8 generated header (`/workspace/build-1214/base-94197fc8/build/gen/x0x_drum_tables.h`) and the **new** decoder from this head's generated header (`build/gen/x0x_drum_tables.h` from the BugBot device build). Gold 19×128 IEEE bits are computed independently from both generators' `pot_exp`/`bits32` (generators agree; gold SHA256 `c8c351223b0c805e68f88c917c5ed0a8b4983277681e2a1b43251627ffbd3faf`). Log: `5849cb11-harness/out/harness-O2.log`.

**(a) Shared pitch table.** The table is `int32_t original_pitch44100[128]` (pitch increments, compared as uint32 bits). Header values at 94197fc8 and at this head are identical (same blob; 128 entries, all positive, strictly increasing). Both backends at 94197fc8 included the same header via `-Iexperiments/original_synth`. Bind validates all 128 entries; `original_pitch_increment` clamps pitch to 0..120 and reads `table[j]` and `table[j+1]` (index up to 121). All 128 words were compared.

Both 94197fc8 Elite and Pro ELFs contain **two** 512 B copies (`original_pitch44100` at `0x02062ab8` and `original_pitch44100.1003` at `0x02062cc8`). The 5849cb11 Elite and Pro ELFs contain **one** copy (`0x02062ad8`). All six dumps are byte-identical to the header (SHA256 `77c39cdfb56f1cd4da2bfbe83d1c7902a0a8078e0b66f98045a49d0e9dc20926`). `ALL_MATCH_HEADER=True` (`harness/out/pitch-elf-compare.txt`).

**(b) EXP pot curves.** All **19×128** values: old decoder == new decoder == gold formula. `BIT_EXACT PASS`. ASan/UBSan in-range also PASS (`harness-asan.log`). All 19 `X0X_EXP_*` names and ranges are unchanged.

**Out-of-range.** Neither decoder clamps (comment: "callers retain their domain checks"). Callers `fxbus_set` and `drum909_set` clamp to `0..ui.max` (127 for EXP pots) and are `X0X_COLD`. At pot=128 and pot=200, reconstructed bits **differ** (different packing past a row). ASan at pot=128 did not fire (still inside the combined word array, including the padding word); last-curve pot=128 on the new decoder happens to add a zero pad and repeat pot=127. A huge `pot` would make the new O(pot) loop run for a long time; that was not executed. In-range identity holds; OOB is out of contract and does not match.

---

## 3. No feature loss — **PASS**

- Pitch: Chicago now `extern const int32_t original_pitch44100[128]`; Miami exports one copy via `__attribute__((alias("original_pitch44100_local")))`. Both bind sites still pass that symbol. No engine dropped.
- EXP: `5849cb11` only changes `tools/gen_drum_samples.py` (storage + `x0x_pot_exp_value`). Every previous call site still goes through `x0x_pot_exp_value`: `fxbus_param_si` / `fxbus_set` (Drive, RvHPF, DlHPF) and `d9_pot_value` used by `drum909_set` / `drum909_param_si` (all 909 EXP macros: BD/SD/LT/MT/HT/RS/CP/OH/CH/CY/SMP, plus Drive). All 19 curves remain. No call-site removals.

---

## 4. Not-slower rule — **FAIL** (blocking)

`x0x_pot_exp_value` at 5849cb11 is `for (unsigned i = 0; i < pot; i++)` (O(pot) integer delta adds). At 94197fc8 / `d99a2588` it is a single packed-bit extract (O(1)). The function is `__attribute__((noinline))`. Callers are `X0X_COLD` (UI/MIDI pot set and UI readout), **not** `fxbus_process` / `drum909_render` / the audio timer path.

**Independent BugBot host microbenchmark** (`cc -O2`, noinline decoders, 200,000 reps × 19 curves; `harness-O2.log`, 06:52 AM America/Chicago / 22:52 +10):

| pot | old ns/call | new ns/call | ratio |
| --- | --- | --- | --- |
| 0 | 1.68 | 1.19 | 0.71× (faster; no loop) |
| 1 | 1.84 | 2.60 | 1.41× |
| 16 | 1.90 | 16.99 | 8.94× |
| 32 | 1.64 | 34.60 | 21.06× |
| 64 | 1.72 | 66.30 | 38.46× |
| 96 | 1.79 | 100.48 | 56.10× |
| 127 | 1.98 | 121.80 | 61.58× |
| full 19×128 sweep | 4.585 µs | 168.882 µs | **36.83×** |

**Optimizer CPU-check log** (read only, `/workspace/x0x37-optimizer/cpu-check-026/host-bench-cputime.log`), verified against the numbers above — same conclusion, close absolute values (different harness, 11-run medians):

- Uniform 19×128: **1.850 → 62.762 ns/call** (+3292.77%). Confirmed as stated.
- pot=0: 1.794 → 1.423 (−20.72%, new faster). pot=1: 1.968 → 2.468. pot=32: 1.791 → 31.689. pot=64: 1.773 → 67.030. pot=96: 1.716 → 94.745. pot=127: **1.977 → 126.839** ns/call. Agrees with BugBot's 1.98 → 121.80 at pot=127 and with the sweep average.
- `original_pitch_increment` (the `d99a2588` path): 3.763 → 3.685 ns/call (−2.07%). **Not slower.**

**Device disassembly** (Optimizer `pot-dis-*.txt` and `dis-norm-compare.log`, read only; agrees with the C source):

- 94197fc8 and `d99a2588` (`pot-dis-base.txt`, `pot-dis-pitch.txt`): straight-line O(1), 28 normalized insns, no loop; the two are the same sequence. Audio-path functions compared (`d9_render`, `run_job`, `fm1_timer5_irq`, `original_pitch_increment`, Chicago/Miami render, …) are **SAME** insn sequences at base, `d99a2588`, and head.
- 5849cb11 (`pot-dis-head.txt`): 33 normalized insns, and a device loop: `if (--r1 != 0) goto 0x20551c8`. `x0x_pot_exp_value` is the DIFF. No device cycle-count evidence (host timing + insn shape only).

The decoder is proven slower. That is a blocking FAIL under the owner not-slower rule.

---

## 5. Size and RAM — **PASS** (device builds finished before the stop)

One Elite and one Pro default build at this head, same `/workspace/build-1214` recipe as the cd9a1948 QA (`X0X_DEV_PADCAL=0`, SDK V1.2.14, toolchain `/home/box/.jieli/toolchain`, `--nightly-version 0.0.26 --git-sha 5849cb114b20212c22d75b28839d021696fd4d68`). `DNBOX_PRESET_MEASURE_STAGE` unset. Source cwd: the worktree. Artifacts copied to `/workspace/qa-runs/5849cb11/artifacts/`. Logs: `logs/elite-build.log`, `logs/pro-build.log`. No flashing.

**Elite** (06:47:06–06:48:40 AM America/Chicago; box 22:47:06–22:48:40 +10), exit 0:

```
unset GIT_DIR GIT_OPTIONAL_LOCKS
cd /workspace/qa-runs/5849cb11
export JIELI_TOOLCHAIN=/home/box/.jieli/toolchain AC79_SDK=/workspace/build-1214/sdk-v1.2.14
export X0X_DEV_PADCAL=0 BOARD=smk37 MODEL=elite X0X_JOBS=4
python3 -u tools/build.py --board smk37 --model elite \
  --sdk /workspace/build-1214/sdk-v1.2.14 \
  --nightly-version 0.0.26 \
  --git-sha 5849cb114b20212c22d75b28839d021696fd4d68
```

**Pro:** same with `MODEL=pro` / `--model pro` (06:49:53–06:50:22 AM America/Chicago), exit 0.

| | 94197fc8 | 5849cb11 | delta |
| --- | --- | --- | --- |
| Elite flash free | 8,440 B | **11,464 B** | **−3,024 B** app (free +3,024) |
| Pro flash free | 8,448 B | **11,472 B** | **−3,024 B** |
| Elite app.bin | 544,232 B | 541,208 B | −3,024 |
| Pro app.bin | 544,224 B | 541,200 B | −3,024 |
| RAM `.data+.bss` | 95,536 B | 95,536 B | 0 |
| `.data` | 588 B at `0x01c08000` | same | 0 |
| `.bss` | 94,944 B at `0x01c08250` | same | 0 |
| `_bss_end` / `_pool_start` / `_pool_end` | `01c1f530` / `01c20000` / `01c71f6c` | same | 0 |
| pool used / free | 335,724 / 8,340 B | same | 0 |

ELF symbols (Elite; Pro matches the same sizes): 94197fc8 has two 512 B pitch copies + `x0x_exp_words` 8,100 B + `x0x_pot_exp_value` 72 B; head has one 512 B pitch copy + `x0x_exp_words` 5,572 B (−2,528) + decoder 88 B (+16). Net −512 −2,528 +16 = **−3,024 B**, and `.text` shrinks by exactly 3,024 B (Elite `0x83ea4` → `0x832d4`). The −3,024 B total claim and "no RAM/pool change" are confirmed. Size PASS does not override the not-slower FAIL.

Elite ELF SHA256 `e71d0dab98ad27dee184d96808fd899632f92d80798e5707aca5a76c930d3b17`; Pro `84eaf6fc5e8b8aac95ef4f7dbf0ec9dbb69af7764eb1de87115ae3faf46489e1`.

---

## 6. Host suite and quickcheck — **NOT_RUN** (stopped; not a pass)

- **`tools/quickcheck.sh`:** **NOT_RUN.** Never started (stopped before launch to save box load).
- **`tests/run_tests.sh` (full host suite):** **NOT_RUN** to completion. Started once at 22:53:55 +10 from the worktree with official refs (`NINEW9_DIR` / `EIGHTW8_DIR` / `SCHWUNG303` / `TB3PO_REF` under `/workspace/x0x37-refs/schwung/`). Killed at ~22:56 +10 on parent request (process tree rooted at the BugBot suite shell only; Nov-08 leftover waiters were not touched). Log: `/workspace/qa-runs/5849cb11/logs/full-suite.log` (header + a stop note; the runner had not yet printed a final summary).

  Partial `.results` written before the kill: **47 `ok`**, **0 FAIL files**. These are an incomplete slice, **not** a suite pass, and are **not** treated as PASS. Names: composed-features, juno6-runtime, juno6-slot, drone-string, juno6-dsp, usb-retry, composed-panic-cutover, panic-clear, composed-ui-identity, poly-chord-integration, project-rates-companion, composed-midi-epoch, break-retrigger-lut, fastmath, fastmath-ubsan, home-tie-semantics, sequencer, seq-chain-atomic, chain-longest-playing, bass-prob-midi, bass-303-adapt, seq-tb3po-bars, seq-random-swing, seq-random-clock-handoff, seq-eighth-quarter, held-chords, queued-mutes, break-rate, seq-long-notes, seq-pingpong-resize, clk-lock, seq-random-tie-preview, encoder, bass-midi-pending-native, usb-product-names, nightly-version, uac, uac-out, uac-out-irq, usb-audio-descriptors, trs, undo, undo-capacity, lz, motion, motion-pattern-handoff, install-slots-web. All remaining tests: **NOT_RUN**.

Optimizer already admitted these two scripts were not finished on their side. BugBot also does not have a finished official run. **NOT_RUN is not pass.**

---

## Verdict

**FAIL** on the not-slower rule for `5849cb11` (`x0x_pot_exp_value` O(pot) loop; host ~1.85 → ~62.8 ns/call uniform, ~2 → ~122–127 ns/call at pot=127; device disassembly shows the loop). `d99a2588` is not slower. Rules, bit-identity, feature-loss, and size/RAM checks that finished are PASS. `quickcheck.sh` and the full host suite are **NOT_RUN**. Harness retained at `/workspace/qa-runs/5849cb11-harness` for the replacement O(1) head.
