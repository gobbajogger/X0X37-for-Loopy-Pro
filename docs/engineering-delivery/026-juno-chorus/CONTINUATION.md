# Juno slot integration checkpoint

Parent: `8108c16dbc531d9d2e833cd3016f507dab0ac724`, PR #867.
Shared slot foundation carried from Drone PR #865 at `d4bef99ff445a09546dacf9b205318fc9dd133a4`, including its reviewed UI correction and integrated regression at `4aa5aaeb20f9c79caa4f0423a3ab956c2af7fa5b`. This is not a merge to the delivery branch.

## Implemented

- Juno is engine kind 3 in either existing synth slot. Drone remains kind 2; Chicago and Miami IDs are unchanged.
- Six fixed voices and one 512-byte chorus ring per selected Juno slot, shared across its voices. The 888-byte Juno state fits the existing 3840-byte backend union.
- Production note ownership, slot selection, render, mute/level/FX buses and four factory patches reuse the existing adapter.
- Eighteen controls include oscillator mix, sub, filter, ADSR, HPF, PWM LFO, shared chorus, level, bend and damper. The final two controls are performance-only.
- Existing 32-byte patch and project layout are retained. Juno per-pattern preset codes are 0xE1..0xE4; Drone 0xC1..0xC4 and legacy Chicago/Miami encodings are unchanged.
- PANIC clears voices, sustain and chorus immediately; normal note-off retains envelope release.
- Fixes defects in the standalone module: sub oscillator now retains its own octave phase; chorus delay never falls below 1.7 ms; unsupported native libm calls use the project's freestanding math.
- Two cold runtime functions use the existing minsize policy. Render and note dispatch retain their normal optimization policy.

The oscillator/filter is original C code informed by classic subtractive/PolyBLEP design, not a claim of a verbatim Plaits port or exact Juno hardware emulation.

## Evidence and remaining work

Focused Juno DSP checks pass with GCC and ASan/UBSan; leak detection disabled in the ptrace executor. Production runtime checks pass including exact-zero stereo after PANIC. Whole-app slot test passes 102 assertions for four presets in both slots, UI identity, note lifecycle, isolation and return to Chicago. Existing Chicago/Miami cell tests pass with native/soft arithmetic and sanitizers.

Native Elite still FAILS the unchanged XIP limit; the current cold-code annotated build overflows by 3344 B (previously 3776 B). No native image is qualified or published. The first full host run was invalidated when a selected rerun cleared its shared runner output. It is not acceptance evidence. A fresh full run uses an isolated frozen-source workspace with the recovered historical Git inputs. Focused standalone results remain separate; no full-suite pass is claimed. Both exact-final native builds, complete host suite, pool/flash evidence and performance qualification remain required. The owner's latest instruction skips emu360 locally; no emulator result is claimed.
