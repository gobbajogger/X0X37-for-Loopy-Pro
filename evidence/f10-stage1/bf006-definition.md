# BF-006: 808 exp2 signed left shift is undefined (2026-10-23)

Status: VERIFIED HOST BUG, UNFIXED ON GIT. Severity: MEDIUM (potential compiler-dependent 808 tuning/initialization); audible device fault NOT demonstrated.

## Exact source / reason
- Public repository main c9718ae833eb64a5191c350922593df8a36017c9; gh-pages dcb41f2ead9e2166ec3dddcd658e445d22a5fac9; neither modified.
- firmware/src/dsp/drum808.c:165 (Git blob 4b873a79cb050a789cfa26c2490ca8cf61c94ecb), in d8_exp2_cr(float x): b.i += (int32_t)n << 23;
- A valid negative input x=-3.0 produces n=-3. Left-shifting a negative signed integer is undefined C behavior. Expected d8_exp2_cr(-3.0f) = 0.125f with no UB; observed two independent host UBSan failures at line 165, before any hardware test.
- This is a DIFFERENT source occurrence from the previously found fastmath.h:57 UB and is not repaired by existing firmware draft PR #28.

## Executed repeatable proof
- Source files for drum808.c/.h, drum909.c/.h, fastmath.h, x0x_param.h, tests/host/drum_pan_test.c and tools/gen_drum_samples.py were checked by immutable Git blob SHA against main. Extracted archive snapshot was historical as a whole; checks do NOT claim an exact current full checkout.
- Runner: Linux x86_64, GCC 14.2 and Clang 17; each compiled the added production-source regression at -O1 with -fsanitize=undefined -fno-sanitize-recover=all.
- Baseline unchanged main: GCC exit 1 and Clang exit 1: drum808.c:165:23: runtime error: left shift of negative value -3.
- Scratch PR #28 fastmath.h replacement alone (exact proposed fastmath blob a5c26c95cad0d91a548a8db073f59777e36d4e86): GCC exit 1 and Clang exit 1 at the same 808 source line.
- Scratch-only two-change counterfactual: above fastmath fix plus replace exactly that 808 line with b.u += (uint32_t)(int32_t)n << 23; GCC and Clang UBSan exit 0; nine exp2f numeric checks PASS (-64 to +64, including -3). No firmware patch committed here.
- Production 808/909 audio-pan host test on unchanged main with sanitizers DISABLED: GCC and Clang exit 0, centred stereo/mono bit identity and hard-left pan PASS.
- Production audio-pan test with PR #28 alone: GCC and Clang ASan+UBSan exit 1 at 808 source line 165.
- Production audio-pan test with both scratch changes: GCC and Clang ASan+UBSan exit 0; all three audio-pan controls PASS.
- Project preflight/staging checks run too: memory-full preflight PASS; staged project 24,446 checks/0 failures including Clang ASan+UBSan. Project runtime baseline PASS; its sanitizer run still fails at the known fastmath.h:57 issue. No new project safety failure proven.

## Reproduce after generating build/gen tables
Compile the new tests/host/bugfinder_drum808_exp2_test.c from repository root with:

    cc -std=c99 -O1 -g -ffp-contract=off -Wall -Wextra -Wno-unused-function -fsanitize=undefined -fno-sanitize-recover=all -Ifirmware/src/dsp -Ifirmware/src -Ibuild/gen tests/host/bugfinder_drum808_exp2_test.c -lm -o build/host/bugfinder_drum808_exp2_test
    build/host/bugfinder_drum808_exp2_test

Regression intentionally fails on main; do not label it suite PASS until the firmware integrator reconciles BOTH unsigned-shift fixes and runs affected checks.

## Ownership / safety
- Bugfinder lane owns this evidence and standalone negative regression only. PR #28 is the active firmware DSP repair lane; firmware integrator owns source correction and Elite/Pro target validation. Do not duplicate PR #28 edits from this branch.
- No exact-current full firmware build, matched controller package, validated target toolchain/SDK/DSP references, memory/pool margins, device test, flash, Pages write, stable release or main merge occurred.
- Existing BF-004 delayed Note Off ownership and BF-005 stale pending MIDI clock bugs remain unresolved. Candidate publication remains NO-GO.

## Practical effect
No new user-visible improvement this run. The 808 may be affected by compiler optimization in a code path that is already exercised by normal initialization; precise audible/device effect is unmeasured. The independent regression will catch this unambiguous UB before firmware validation.
