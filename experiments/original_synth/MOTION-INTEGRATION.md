# Original synth motion and Chicago filter checkpoint

This is the same original-port continuation after accepted VCA correction `5883989`.
It adds reference-derived components, not an active backend or a complete port.
No `engine.c`, UI, SMC routing, boot, storage or existing sound path is changed.

## New closed behavior

- Miami double XMod and independently traced Chicago SYNC/XMod use signed integer phases, saturating FCVTZS, unsigned modulo subtraction, strict pulse wrap, and original triangle/saw/pulse equations. Chicago resets each slave to `0x7fffffff` on signed master wrap; Miami does not.
- Original increment-derived waveform coefficients are prepared at control rate. The prepared sample API uses immutable validated preparation. Reprepare after changing any increment, selector, width, gain or XMod control. Do not directly mutate a prepared object. The convenience API prepares each sample only as a checked reference/test route.
- MG tempo uses the original eight native denominator products and separately rounded arithmetic. Evaluate the waveform from the old phase first, then advance. This does not invent unison phase initialization or replace previous-tick modulation snapshots.
- Four-pitch glide uses exact fused original arithmetic. PW and XMod preparation and the original 44.1-kHz integer pitch lookup are explicit helpers, not the missing complete four-oscillator note/pitch binding.
- Chicago MP4 uses the traced five-state recurrence, polynomial feedback, A/E/R ramps, caller-requested R^8 initialization kick and separate postgain factor. It does not implement Bite, ARP, pan, allocator or note lifecycle.
- The private generator consumes immutable correction receipts. Cap indices 108/109 are corrected. Miami A is positive using corrected subtraction order. Miami A/B/R are locally derived libm values, not initialized plugin-memory readbacks. Chicago A/E are transferred original-recipe derived arrays, likewise not live memory. Chicago ARP remains 28 ON / 53 OFF.

## Inputs and numerical contract

Original Miami core SHA256: `bb33879a6bad7917001d70414d558391ba6252382c54072c5f324ca8d16064ca`.
Original Chicago core SHA256: `4aa18347043539f8a87813c46831bc451eb33aaca5c1cabcb036363b71b07009`.
The generators pin each complete private manifest and table payload hash. Earlier reports are unchanged; corrections take precedence only in this successor.

Require binary32 RNE, gradual underflow, masked exceptions, no fast-math and no FMA contraction except explicit fused calls. The accepted exact software FMA is the target baseline. Host/live AU FPCR and exact cross-libm table equivalence remain unverified. No approximation or missing-coefficient substitution is used.

State is explicit: oscillator constructor seeds and original analog offsets are still unresolved. The joined host case uses clearly synthetic supplied phases and controls. It is not a named original preset, acoustic acceptance or complete note event.

## Proof and cost

`tests/host/original_motion_vectors.py` creates independent operation-order fixtures from the pinned reference recipes and original derived tables. `original_motion_test.sh` runs native-fmaf and exact-software-FMA variants, each normal and ASan/UBSan:

- 29,344 waveform cases, including strict pulse wrap and correction threshold.
- 8,192 four-oscillator phase/mix frames for both engines; prepared and convenience paths match exactly.
- 6,816 MP4 state frames with original derived tables, coefficient ramps and initialization kicks.
- 8,192 four-pitch glide ticks, 32,768 MG phase ticks, pitch/PW/XMod edge checks.
- A joined 44,100-frame explicit-phase Miami oscillator/MG/MS20 path, finite output and bounded state.
- 262,144 same-input oscillator frames compare repeated preparation against control-rate preparation. This measures host implementation cost only, not the owner's device load or a speedup against the old experimental sound engine.

Host state: oscillator phases 16 bytes, prepared controls 92 bytes, Chicago filter 60 bytes, accepted Miami filter 72 bytes. Private 44.1-kHz coefficient arrays require 133,656 bytes, predominantly Chicago A/E (131,080 bytes). Native object sections confirm this readonly footprint; it must not be silently charged to RAM or assumed to fit the final application. No whole-firmware link or native runtime timing has run for this checkpoint.

## End-to-end continuation

Keep these components inactive until the same reviewer accepts this exact candidate and remaining integration invariants close. Continue the existing original-reference task for constructor phase seeds, analog/per-key offsets, complete four-oscillator pitch binding, sample-rate coordination, Chicago Bite/ARP and outer FX lifecycle. Preserve engine queue ordering, previous-tick modulation source snapshots, two-lane unison allocation and whole-note sound lifetime.

Factory selectors must retain the accepted typed-generation bridge; original stock vectors must not round-trip through legacy bytes. Edited legacy presets retain their meaning, with no automatic project migration. Active engine integration and a named factory musical comparison require complete context and correct persistence. Exact source review, native whole-app qualification, handoff, publication and device acceptance remain separate states.

Private tables and original assets have no public publication permission in this checkpoint. Existing actual public-write holds remain.
