# Audio command queue ISR publication repair

A main-loop producer can select a full-ring victim and then be interrupted by
ALNK0. If the audio consumer drains that victim before publication, the producer
writes behind the reader and silently loses the accepted critical event. This
is a source-derived reproducible risk, not an observed hardware symptom.

The repair scans with interrupts enabled, masks only ALNK0 around a fresh
reader-position check and one aligned replacement word, then restores the exact
prior ALNK0 enable bit. If the selected slot was consumed, retry uses the existing
normal SPSC append path once space is available. No queue size, MIDI MQ/realtime,
release priority, sound parameters, REC schema or DSP changes are made. The
reserved C_REC value/priority matches the independently reviewed atomic REC unit;
this branch alone does not implement its producer or playback behavior.

`fm1_irq.h` defines the per-source enable bit and source-mask helper; the actual
consumer is `fm1_alnk0_irq` → `render_block` → `engine_render` → `drain` on CPU0.
`plat_cq_lock/unlock` uses that enable bit, with CSYNC/compiler barriers. It does
not use the old flash `irq_save/restore`, whose restore unconditionally STI's.
Other source priorities/enables and the caller's global interrupt state remain
unchanged. The same-core model assumes a disabled ALNK0 source defers pending
ALNK0 dispatch until reenabled; no cross-core queue consumer exists in this tree.
Device timing/pending-interrupt qualification remains a hardware gate.

The adopted test originated as test-only draft PR621, exact head
`6f9090cf0c841a68a6652f918fa7d4a1262f46cb`, base
`d633833ac369f8466bc2563791614206070e87d1`. Its original strict case loses DRUM
after one consumed entry. The corrected extraction also includes the target
platform lock/unlock and HAL mask C, replacing only MMIO register access and
CSYNC with a sequential interrupt model. Nine scheduling sites cover before
lock, saved enable before mask, after mask, after lock, after revalidation,
before publication and before/after restore. It covers original32 scenarios
plus C_REC, both victim tiers, uint32 counter wrap and nested disabled state.
Omitting either mask or live revalidation must fail. Sequential priority tests
remain separately required; this model does not prove device callback timing.

The first macOS live-REC compile encountered the known hidden libproc feature
macro issue from unchanged baseline harness code. Passing `_DARWIN_C_SOURCE`
via CC exposed those declarations; actual rec-live and rec-live-smk then passed.
No test source was changed to obtain that result. Both are baseline live-REC
fixtures, not proof that the separate endpoint unit is integrated here.

Implementation host proof and target object inspection are recorded under
`cq-isr-proof`. Independent review, combined-source native budgets/full applicable
regression and hardware qualification remain required before integration/readiness.
No source push, merge, package publication or device operation is performed here.

Target object inspection used the exact archived frozen source95e3cca3 with
production `build_app` Elite flags (`-Oz` unity platform, `-O2` engine, PAD1,
WOBBLE1, r3 FPU). The captured native log shows source masking at0x1EEF104,
CSYNC and exact enable-bit restore without STI. Queue code rechecks the live
reader after locking, writes one aligned word only on the live branch, then
restores; consumed candidates restore and retry. Relocatable call destinations
remain unresolved until the separate combined final-link gate. This is object
compilation/inspection evidence, not a full package or native capacity result.
