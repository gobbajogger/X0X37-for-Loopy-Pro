# Independent source review

Reviewer: the read-only `review_909` subagent in this Codex task. The reviewer ran no builds or tests and changed no files. This was ordinary independent source QA, not a requested Sol Loop or hardware review.

The reviewer found no definite arithmetic/state regression in the five optimizer changes. The rim fast path retains two zero-input filter ticks; the settled-pan path retains signed-zero normalization; bass-drum hold reuse calculates the first decay increment before updating frequency. The hoisted controls follow the between-block update contract.

R909-1: Resolved. Pin the test to a06e20189 and use fixture-only identical mute-state updates because that base predates the public panic helper.

R909-2: Measured native app growth is 736 B, with zero static RAM and .ram_text growth. The compiler's local stack allocations increase from 136 to 168 B in d9_render and from 36 to 80 B in d9_render_clap; d9_emit additionally saves six registers (24 B). These are local disassembly observations, not a whole-call-graph stack bound or a runtime high-water measurement. Persistent drum909_t state remains 7648 B on this host.

R909-3: Resolved. Register the equivalence script in the full runner, class G and needs_gen, and generate missing samples/tables on standalone runs. The adapted runner disables contraction and fast math at both optimization levels. Follow-up review found no blocking issue in these adaptations.
