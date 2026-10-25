# Independent experimental host QA

2026-10-25. A separate audio-methodology reviewer inspected source and analysis,
then independently reran the compiled host tests. The DSP writer did not provide
its own independent acceptance.

Accepted scope: deterministic experimental host DSP, bounded state and buffers,
gate cleanup, working renders, meaningful separated reference measurements.
No fidelity, integration, target-fit, deadline or hardware acceptance.

All host behavioral tests reran successfully; state144B, object4344B including
3656B executable text, no undefined object symbols. No material memory/gate defect
was found. The reviewer confirmed pitch cap0.12×sample-rate and unquantified PM
aliasing as limits, now stated explicitly in README and measurements.

The reviewer identified a spectral acceptance defect in the initial analyzer:
requiring a partial strong in both signals could hide a missing reference partial.
The final analyzer uses a union mask for spectral errors, intersection for phase,
asserts stereo reference repeat equality, and refuses silent pitch acceptance.
Regression tests cover missing/added partials and separate gain from spectrum.

Reviewer confirmed all seven fresh pitch goals pass and all envelope/spectrum
goals fail. Pulse RMS+7.19dB/correlation0.362, mixed depth+3.84dB/correlation0.832
are material mismatches. Prototype retained phase does not reproduce carrier
H3/H5 changes of−4.093/−19.782dB or square−4.184/−22.765dB. No architecture inference
or broad tolerance excuses those failures.

Next scope: original pulse-shape/common-gain/filter refinement with holdouts;
keep deterministic spectral-history mismatch open. No denial, pending operation
or uncertain outcome was reported by this reviewer.
