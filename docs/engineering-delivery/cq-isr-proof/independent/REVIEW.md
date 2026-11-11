# Independent CQ ISR source/host review

Frozen production head `95e3cca30816b453d63b90254d5dc7614c40946d`, tree `1bd5cbe809c68ed70e55976fac8850c16e5aac6e`, base `d633833ac369f8466bc2563791614206070e87d1`. Reviewer made no source/test edits. Author added a test-only exact-word assertion supplement after freezing production; its separate working blob is recorded in receipt.json.

## Bounded result

No source correctness blocker found. The full-ring victim scan occurs with the audio source enabled. `cq_replace` masks only ALNK0, reads fresh cq_r, uses wrap-safe interval membership `i-r < w-r`, publishes one aligned uint32 word while the sole consumer is excluded, then restores the exact prior ALNK0 enable bit. A consumed victim retries through ordinary append once room exists. A full protected queue rechecks whether the consumer advanced before returning. No long scan is inside the masked interval.

The platform mask is CPU0 IRQ11 enablebit0x1000 in ICFG register0x1EEF104. The existing HAL mask changes just that bit; CSYNC/compiler memory barriers bracket publication and reenable. Restore uses current config for other fields and the saved ALNK bit; no CLI/STI/global-state changes occur. In-tree target call closure is fm1_alnk0_irq→render_block→engine_render→drain; no second-core CQ consumer appears. This relies on same-core source disabling preventing/defering ALNK dispatch, no reentrant producer, and the existing uint32 SPSC ordering assumptions. Host simulation is explicitly single-threaded and its no-op lock is not threaded synchronization.

## Independently executed proof

- Frozen extraction: ASan/UBSan1080 schedule cases +4 nested enabled/disabled/global-state cases PASS; omitted-source-mask and omitted-live-revalidation mutants both expose lost DRUM and are rejected.
- Author's separate exact packed-word assertion supplement: independently reviewed and rerun with ASan/UBSan, same1080+4PASS and both mutants rejected. C_REC a=0xA5 and remaining packed fields must survive exactly.
- Independent36-case probe: full queue of protected C_BOFF releases; consumer frees1/512/1024entries at scan-end; DRUM/BON/BRK/REC insertion survives at starts0,CQ-3,UINT32_MAX-3. PASS under ASan/UBSan.
- Original PR621 test blob968e485c against exact d633 engine: strict-red exit1 as expected, DRUM lost after one consumed entry. Not represented as a passing implementation test.
- Five adjacent CQ overflow selectors and both baseline liveREC fixtures PASS. Darwin feature macro only exposes existing libproc declarations; no fixture source patched.

## Target object inspection

Author compilation of the frozen archived sources completed exit0 in the pinned JieLi image (receipt/image/compiler hashes recorded in receipt.json). Reviewer independently matched source SHA256 and inspected the resulting target disassembly. Actual production-derived flags were -Oz for unity platform, -O2 for engine, pi32v2/r3/FPU, SMK Elite/PADCAL1/WOBBLE1.

Platform output: ICFG0x1EEF104 source enablebit0x1000 clear, CSYNC, return saved enablebit. Unlock emits CSYNC, current-register read/mask/or saved state/store, CSYNC; no STI/global-state restore.

Engine output: scans54–a2 outside the masked critical region; lockcalla4; fresh cq_r reada8; unsigned modulo membershipac–b0; aligned word store `[r12+r7<<2]=r8` atd4, then unlock tailcallda. The nonlive branch unlocksb4 then rechecks occupancy/retriesb8. Append wordstorec8 precedes cq_w publicationce. Relocatable -d calls do not name unresolved callee symbols; source closure and compiler ordering were inspected, not final linked call addresses.

**PASS for this bounded source/host/target-object review.** No full Elite/Pro linked native capacity, combined REC schema, controller/package/installability, physical interrupt pending/timing, hardware, flash, merge or publication claim. Exact combined-source qualification remains required.

## Final evidence-head closure

Head `63519957c13a4dfbe48c1d696ce0565248f391d7`, tree `e42ca7e47bacf219cec96fb7b418e70abe569305` independently checked. Production source delta from95e3 is empty. The sole test delta is the already independently executed exact-word assertion supplement, blob `025da690e927d179238d3b5cb1f45552ad8fa69d`; remaining changes record the same target logs/receipt and describe their limits. Worktree clean. Bounded PASS retained without redundant host/native reruns.
