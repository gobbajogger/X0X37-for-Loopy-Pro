# Miami zero-XMOD assessment

Request: LX-REQ-023, related LX-REQ-016. This is the already-agreed assessment
and focused testing of `original_miami_voice_render_add`, released by the
existing producer through the parent Work thread. No production-source change.

Inputs: firmware `8be16486e33f65b31c76710e1d825b6aa701c2c9`, tree
`f952ab4455723e4a1a68fdb3a8eea22d2da59710`, kernel
`experiments/original_synth/miami_ms20.c` blob
`f74384b3f5e8850339fe4d42309b36fc10dd6b3b`.

Ownership checked against coordination
`4a35a6cdaa0f112db6b3f577542138afd170c20d` and live PR #814. The existing
producer owns `opt/miami-coef-cache-0.0.24`; independent QA remains
`independent_qa/6adafbbf54088191a86715d49e7dc6b0`. That review is not reassigned.
This lane owns only the new assessment harness and its evidence. The sole
firmware integrator and ledger writer retain their roles.

Execute:

1. Authenticate source pins and current ownership; reuse the supplied
   996,430-case phase arithmetic result without rerunning it. Its original
   artifact/command is not available in this execution context, so do not claim
   independent authentication of that receipt.
2. Execute a baseline-versus-baseline kernel fixture through the selected
   backend, comparing status, filter/gain/phase state and stereo samples.
3. Change only the slave phase subtraction to addition in a generated temporary
   mutant; require a witnessed phase-state mismatch and a nonzero exit.
4. Count zero-XMOD pair samples and current nonzero carrier gains in a generated
   instrumented copy of the exact kernel. Use factory 0 (backend default) and
   factory index 15, note 60/velocity 100, 128 blocks of 256 frames at 44.1 kHz,
   120 BPM. Compare every instrumented kernel call and complete backend PCM.
   Counters describe this workload, not all presets, notes or perceptual audibility.
5. Obtain an independent review of the new artifact only. Deliver reviewed
   nonsecret tests/results on this isolated non-main branch with a draft PR,
   verified remote readback and concrete handoff to the existing producer.

Stop at this bounded assessment and handoff. Do not optimize source, measure
timing, claim a shared performance slot, build/publish firmware, run GitHub
Actions, merge, force push, use Safari or operate hardware. Native builds and
device results are not acceptance criteria for this evidence-only lane.
