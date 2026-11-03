# Timer-free Miami span observation

This continues the released disposable attribution scope for LX-REQ-023, related
to LX-REQ-016. It does not change player behavior or optimize production code.
The existing performance producer and PR #814 reviewer keep their assignments.

Firmware pin: `8be16486e33f65b31c76710e1d825b6aa701c2c9`; tree
`f952ab4455723e4a1a68fdb3a8eea22d2da59710`; backend blob
`2efde1794c8454b7af0f9c527f49abd03a28a783`; kernel blob
`f74384b3f5e8850339fe4d42309b36fc10dd6b3b`.
Reuse PR #830 at `e1c6b02bf6660142af3ae575a3b159cd051a011a` without editing
its fixture, runner or evidence. Its README explicitly says the older original
fixtures were unavailable. No separately identifiable stage-3 fixture has been
recovered from the supplied workspace, GitHub paths, Library searches or personal
context (unavailable). Do not claim authentication or reuse of those missing bytes.
The accessible PR #830 fixture is the unchanged comparison baseline for this work.

1. Verify the pins, policy `nightly@66073c68dd80d51a7053432c2c161a8b720a1341`,
   coordination `4a35a6cdaa0f112db6b3f577542138afd170c20d`, and live PR ownership.
2. Keep new work in this isolated branch. Own only the new span fixture, runner
   and this evidence directory. No shared ledger or producer-branch writes.
3. Characterize the unchanged PR #830 fixture first. Reuse its stored zero-XMOD
   result, arithmetic receipt and negative mutant; do not repeat those checks.
4. In a generated temporary backend copy, count rendered span lengths, active
   voice calls and eligibility of every 64th rendered span (one-based ordinals).
   Include both active voices in an eligible span. Add no clocks or timed run.
5. Reuse the original C fixture in one process: unmodified backend first,
   observational backend second, with the same initialized backend address.
   Compare every kernel call's order/voice/frame count, the existing per-call
   filter/gain/phase/sample checks, all 128 complete backend state images and
   every PCM byte. Match the stored PR #830 PCM hashes. Test eligibility edges
   and demonstrate that a changed span observation is rejected.
6. Preserve one independent artifact-review budget, as in PR #830. Hold the
   artifacts fixed during that review. Do not claim a full multi-reviewer or
   cross-model CE pass. Correct blocking findings before delivery.
7. Push only reviewed nonsecret text artifacts using the connected account's
   default author to `gobbajogger/X0X37-for-Loopy-Pro`. Open a draft PR stacked
   on the unchanged PR #830 branch, with exact remote tree/ref/byte readback.
   Return the evidence and proposed timing boundaries to the existing owner.

Stop at reviewed fixture delivery or a precise action-specific blocker. No
benchmark before a separately recorded actual global slot reservation. No timer,
hardware, Safari, Actions, firmware build, production edit, merge, force push or
publication. A proposal or quiet executor does not reserve a timing slot.
