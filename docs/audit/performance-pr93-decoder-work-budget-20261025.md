# Performance Engineering R11: deterministic BREAK decoder work per 64-frame callback

**End-user behavior:** reverse break loops must remain free of audible discontinuities through cache boundaries, live reverse release and NoteOff events; transport stop must produce silence. An optimization must not trade one long-loop win for unrecognized short-loop danger. Host wall-clock p99 is too noisy to claim device ISR timing; count actual production IMA nibbles instead.

## Exact evidence and scope

- Current own PR #93 BEFORE this evidence-only commit: `947e5bba8fb42f13ea72aedee94276cde40e3acc`, full root `3891fef6e75a9690d5802c2fd5569fa66d837081`, compiler `firmware/` `a87a83e15116422eed84528d95f52ddef5029927`.
- Protected accepted firmware NIGHTLY baseline `54f2ffbc3124f7449a69067dc8215a2d91a2e675`, full root `3b5cadafba33f4c1b116660bc9d19a27a892337c`, compiler `firmware/` `5c209609b770fd13949230aeb80956594fd6b741`.
- All six required production BREAK DSP/decoder/dependency blobs compared to authenticated Git objects for **each** source. The native runner fails closed on a changed source blob, so these results are not reused for another firmware revision.
- Temporary instrumentation adds ONE `volatile uint64_t` increment to function entry of the IDENTICAL upstream `ima_decode` implementation on both sides, preserving the rest of the code. This makes decoder operation counts deterministic but **cannot** measure actual uninstrumented target CPU cycles. No tracked firmware file changed.
- Real linked production `breaks.c`, GCC and Clang with optimized `-O2` and `-O1 -fsanitize=address,undefined` all compiled/run on Linux x86-64 with exit 0. Each of 4 configurations executed 60 distinct before/after cases; invalid selector exit 2. Full generated PCM trace (1,474,560 bytes per executable) exact raw SHA-256 `74ce8b7b436275142a998f4a911340f4157168728448371c1daf722168fee7c1` matches baseline/candidate and GCC/Clang/sanitizers. Both intentionally altered FNV and trace SHA comparator checks rejected corrupt outputs. Source harness includes mid-render live reverse-release, key NoteOff, short/medium/long loops, deterministic synthetic nibble data and post-STOP silence.

## Observed deterministic per-block work (combined two trials)

| 64-frame workload | Baseline nibbles decoded | PR93 nibbles decoded | Saved | Worst one block, baseline -> PR93 |
| --- | ---: | ---: | ---: | ---: |
| Reverse 65,535 samples / 16× | 197,882 | 197,882 | 0.00% | 2,298 -> 2,298 |
| Reverse 176,400 samples / 16× | 201,319 | 201,319 | 0.00% | 2,151 -> 2,151 |
| Reverse 2,116,800 samples / 1× | 87,961 | 54,169 | 38.42% | 8,192 -> 8,192 |
| Reverse 2,116,800 samples / 16× | 1,308,569 | 789,401 | 39.67% | 15,872 -> 8,704 |
| Reverse 2,116,800 samples / 64× | 5,254,041 | 3,175,321 | 39.56% | 47,104 -> 28,672 |
| Reverse 2,116,800 samples / 256× | 20,934,857 | 12,649,673 | 39.58% | 151,753 -> 100,553 |

**Interpretation:** Both short/medium loop workloads have exactly **zero change in decoder calls**; therefore their mixed GCC host p99 timing cannot be attributed to decoding fewer/more IMA nibbles. The long 2,116,800-sample reverse workload saves 38–40% decoder calls across tested rates, while an isolated 1× 64-frame callback can still require 8,192 calls on BOTH implementations. The extreme 256× synthetic rate still consumes tens of thousands of decodes per callback, not proven MCU-safe. Neither CPU clock budget nor audible underruns were measured on a controller.

**Reproduction:** The private GPL-source-only evidence archive includes exact Git-blob-matched baseline/candidate C/H source projections and `reproduce.sh`. After extraction, run `bash reproduce.sh`; it runs both GCC/Clang optimized and ASan/UBSan checks with fresh output paths. In independent full checkouts, the committed `tests/host/run_breaks_decoder_work_audit.py` accepts separate `--baseline-root`, `--candidate-root`, `--output-dir` and `--compiler both` options. Supply separate complete source checkouts or exact Git-byte projections; all six source objects are byte pinned. Previous tested runs were from exact six-file source projections, **not** complete repo checkouts. Runner creates only temporary instrumented copies, writes a structured `result.json`, and never produces firmware/controller packages. The real commands, logs and per-file SHA-256 digests are in the private reproducible evidence archive and adjacent JSON manifest.

## Disposition and ownership

**Producer scoped host PASS only.** PR93 remains optional, non-draft and not self-approved. The stable accepted NIGHTLY candidate is unchanged. Existing Independent QA consumes exact PR93 HEAD and checks production phase/NoteOff and worst callback budget independently before possible sole Safe Merge consideration. Previously pinned Elite+Pro/controller BUILD_ONLY remains separate, pool free 8,308 B >= 8,192 B; no new compiled source/tool inputs or new model build in this test-only change. Never interpret host operation counts as physical XRUN certification, or fold optional PR93 into protected NIGHTLY without approved QA and merger.
