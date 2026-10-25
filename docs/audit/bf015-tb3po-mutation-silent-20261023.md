# BF-015 — TB-3PO mutation can silence the last generated note

**Severity:** MEDIUM (audible 303 generator regression, low-density edge case).
**Source:** firmware NIGHTLY 58b5b45a2dd90581de7a674c331ca5cbe8a67c7e.
**Source blob before:** firmware/src/seq/tb3po.c = 63018311bf3bb86d6a3638f4935ae9cc9dec0cb2.
**Scope:** only TB-3PO mutation; no SMC, clock transport, installer, sample data, flash, hardware or website changes.

## Verified before

The generator supplies an audible root note even when density is zero, but mutation may replace its last gate with a rest. With one step, density 0 and RNG seed 1, the very first mutation silences the pattern. At zero density later mutations almost never restore the note (the 24-bit random draw can exceptionally be exactly zero).

New regression tests compiled from the exact baseline source blob. GCC 14.2 and Clang 17.0 with -O0 and -O2, -fsanitize=undefined: all 4 baseline runs exited 1 with "silent generated line len=1 seed=1 mutation=0". This is an expected negative reproduction, not a compiler crash.

## Corrected behavior and native checks

Track whether a gate existed before mutation and after mutation. Only when mutation removed the last gate, restore the root note on step 0 with default velocity. The deterministic manual all-rest regression stays silent across its 512 selected mutations; exceptionally rare zero-valued draws are a separate unchanged edge case. The correction adds no PRNG draws and does not change RNG order/seed or step probability.

With GCC and Clang, -O0 and -O2, -Wall -Wextra -Werror -fsanitize=address,undefined and -fno-sanitize-recover=all: 4/4 corrected runs exit 0. Each run checked density=0 generated patterns with 1, 16 and 64 steps and seeds 1, 91 and 113 for 4096 mutations per configuration; a manually cleared silent line across 512 mutations; and density=100 at 64 steps across 512 mutations.

The existing sequencer host suite (GCC and Clang, -O2) passed both native runs (seq: ok) against unchanged sequencer and pattern source blobs matching current NIGHTLY. The separate format-2 303 migration shell test passed once. Total executed runs: eleven (4 negative + 4 positive + 2 sequencer + 1 migration). Optional external schwung-tb3po reference matching was not run.

## Provenance, limits and handoff

The local source bootstrap contains historical Git commit f58d62f0b622b9c5792bda81793e0031d8be8747, not live NIGHTLY HEAD. The corrected tb3po.c and its test compiled locally match the blobs committed to this draft PR; relevant unchanged sequencer source and test blobs were independently compared with the live nightly branch. Whole-tree exact-source build, real Elite/Pro + controller compile, RAM/flash budgets, artifact hash/CRC and physical hardware validation are NOT RUN. This is HOST CHECKED, SOURCE ONLY, BUILD PENDING, DEVICE NOT TESTED, NONINSTALLABLE.

Consumers: Independent QA must reproduce at PR HEAD and inspect effect on deterministic reference sequences. Build-first Hourly Build may compile exact PR HEAD for Elite/Pro and controller BUILT_ONLY immediately, independently of QA, without treating compilation as installable acceptance. Sole Safe Merge evaluates the source integration into firmware nightly. No change to firmware NIGHTLY HEAD/main/gh-pages nor publication/flash is authorized here.
