# Optimization Watch — BREAK waveform peak wraps to silence at negative full scale

**2026-10-23 · P2 display correctness · documentation-only research · firmware/device NOT modified or tested**

## Executive summary and user relevance

X0X37's BREAK loop waveform overview can display a completely blank 232-column waveform for an IMA ADPCM loop that is continuously at negative full scale, although an equivalent positive-full-scale loop displays all 232 columns at maximum height. The loss occurs **in the visualization**, not in verified audio playback: the peak magnitude `abs(-32768) = 32768` is shifted right seven places to `256`, then narrowed to an 8-bit pixel value of `0`. The UI skips columns whose resulting height is zero. A loop with even one decoded -32768 sample in a peak bucket can therefore have that entire high-energy bucket hidden. This may mislead users browsing or choosing BREAK slices in a live set.

The synthetic extreme input is valid for the decoder's saturation arithmetic. **Incidence in ordinary owner-loaded samples, actual audio output, on-device visualization and audio XRUNs were not measured.** No sample/slot read or write occurred.

## Exact source, ownership and reachability

- Checked live production **`main@c9718ae833eb64a5191c350922593df8a36017c9`**; [`firmware/src/app/engine.c` lines 1042–1082](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/engine.c#L1042-L1082), Git blob `555480d9c81ca8b088842bfa6bac93e85865d49a`.
- The `engine_brk_outline()` code explicitly clamps negative decoded IMA values to **-32768**, accumulates positive absolute peak magnitude in signed 32-bit `pk`, and writes `(uint8_t)(pk >> 7)` at **both** intermediate and final column paths (lines 1072 and 1079). `32768 >> 7 == 256`; unsigned 8-bit narrowing maps 256 to zero. The opposite extreme +32767 maps to 255.
- The live [BREAK UI](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/app/ui.c#L3567-L3591) requests `engine_brk_outline(0, ui.outline, 232)` and draws a column only when `h = ui.outline[x] * 26 / 255` is nonzero; `ui.c` Git blob `0c661c870f537348070fd3aaff665ced3bc97cd0`.
- [`brk_bank_t`'s consumed layout](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/c9718ae833eb64a5191c350922593df8a36017c9/firmware/src/dsp/breaks.h#L44-L55) confirms the test stubs only the required `adpcm` and `n` fields; header blob `094af6650333e844311a8806145a6177a0318225`.
- Searched open/closed repository PR titles for `outline`, `waveform`, `peak`, and `ADPCM`: no matching report/repair identified. Existing Bugfinder and DSP source branches remain independently owned. This report changes **no firmware/installer source**.

## Reproduction and actual native check exits

From the verified GitHub `engine.c` blob, extracted **exact, unedited source** starting at `static const int16_t ISTEP[89]` and ending immediately before `float engine_gr_db(`. The 1,743-byte excerpt has SHA-256 `12045f94cf5e250019525a19fc737ad1580e0dd4772c08fb05670f3389219f37`, FNV-1a32 `c1a2d155`. A standalone harness compiles this production excerpt with minimal `brk_bank_t`/global state and deterministic IMA data: `n=232` columns, `7,424` decoded samples, 3,712 packed bytes. Synthetic loops repeat nibble `0x7` (positive), nibble `0xF` (negative), or `0x0` (silence). This measures the **visualization encoder only**, not target audio or playback.

```text
Linux x86_64, GCC 14.2.0, Clang 17.0.0
gcc   -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer outline_harness.c -o outline_gcc
clang -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer outline_harness.c -o outline_clang
GCC build exit 0; GCC ASan+UBSan run exit 0
Clang build exit 0; Clang ASan+UBSan run exit 0

                            first  last  nonzero / 232
positive 0x77               255   255   232
negative 0xFF                 0     0     0
quiet 0x00                    0     0     0
```

The harness asserted the expected **broken current behavior** and exited 0 because it reproduced the defect. This is not a correctness PASS for current visualization.

**Fix proposal only, not a firmware change:** clamp the pre-narrowing magnitude at 255, e.g. `(uint8_t)(pk >= 32768 ? 255 : pk >> 7)`, at **both** stores, and cover positive-full-scale, negative-full-scale, silence, and mixed-bin cases. A local **unapplied proposal simulation** on a copied excerpt compiled and passed the same GCC/Clang ASan+UBSan checks (all four compile/run exits 0): positive 232/232 visible; negative 232/232 visible; silence 0/232 visible. These checks do **not** constitute a current-tree firmware build, target budget check, or on-device test.

Harness SHA-256: `f13940e33704e684b11d855b46d8dc55d723e58f0562834da07407cb2cd077d7`. Proposal snippet SHA-256: `03c0a97394a995c3ea7df05243f1bf10b1043c8c502fea5914d23880c33dce2c`. Proposal test harness SHA-256: `9ab50d6b8a1e2930450c64d0e9c638a42691dffdc7d0b426d179983284840d25`.

## Disposition, receiving owner, and genuine unknowns

- **Next consumer:** Bugfinder for a narrowly scoped visualization regression/source fix; Independent QA to verify source identity and both write sites. Only the designated integration owner can advance reviewed source. Do **not** mistake this documentation-only report for a fix.
- **Performance scope:** the loop-outline decoder runs when the loop changes, not in the audio render path; no audio timing or DSP optimization is justified by this issue. Preserve the existing cache/recompute policy and avoid unnecessary audio-path changes.
- **Unverified:** real imported loop amplitudes, actual owner hardware visual behavior, source-tree integrated regression, Elite/Pro build budgets, Loopy timing, physical slots, audio XRUNs. The synthetic test does not prove occurrence under typical musical samples.
- **Channel boundary:** neither NIGHTLY nor PRODUCTION publishable firmware criteria change. No code, binaries, private DSP/SDK, samples, catalogs, `main`, `gh-pages`, firmware packages, or hardware were touched.

**Stage:** SOURCE-MATCHED HOST REPRODUCTION / DOCS-ONLY AUDIT. Reviewer action remains optional investigation, not product release approval.
