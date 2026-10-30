# Chicago effect repair evidence

Baseline: `7e4bbfd95da1d9ad86c80541b9bceae48231d006`.
This is the DSP unit of the owner's requested firmware bug sweep. It preserves
the current source integration owner and existing LX-REQ-016 qualification flow.

## Reproductions and repairs

- BPM Delay always used the offline 170 BPM reference. At 120 BPM its first
  division used 1,946 samples instead of 2,756; at 240 BPM it used 1,946 instead
  of 1,378. Runtime programming and render-boundary refresh now use the effective
  sequencer tempo. The fixed-tempo offline comparison API is retained.
- Every effect control write reinitialized the selected processor. A stored
  impulse produced echo energy 1.72245069 without a write and zero after either
  an unchanged wet-pot write or a 127-to-126 edit. Initialization now occurs on
  actual algorithm changes; coefficient edits preserve the running history.
- Both engine configurations skipped inserts and mixing once native303 became
  idle. After a real NoteOff the baseline retained ring energy 0.168761805 but
  emitted zero during the next 51,200 rendered frames, with unchanged ring
  position and contents. Active Chicago inserts now process zero dry input and
  mix their tails; Off retains the idle shortcut and Miami stays on its own DSP.

## Executed proof

The new fixture uses the complete production engine, normal host platform,
actual command queue and actual MIDI clock input. A separate observation
translation unit exposes effect state without replacing its implementation.

`sh tests/host/run_chicago_fx_runtime.sh` compiles with GCC 13.3.0 at `-O2`,
`-ffp-contract=off`, C99 and the host's warning flags. It checks WBL=0 and WBL=1,
with preset preview and tagged sounds enabled and note-sound capture disabled.
Known baseline weak-inline/attribute warnings remain advisory; this does not
claim the separately owned strict-host repair from PR651.

| Exact check | Result |
| --- | --- |
| Same new fixtures copied into an archive of the baseline, production source unchanged | Exit 1; WBL=0 ran 51 checks with 42 failures. The fail-fast script did not run WBL=1 in that baseline invocation. |
| Candidate runtime, WBL=0 | Exit 0; 51 checks, no failures. |
| Candidate runtime, WBL=1 | Exit 0; 54 checks, no failures. |
| Internal tempo 20, 120, 170, 240 and 300 BPM | Correct expected delay lengths, including bounded-buffer division folding. |
| Real incoming MIDI Clock at 90 BPM | Effective tempo 90.0000; delay 3,675 samples. |
| Repeated TYPE/EDIT1/EDIT2 writes across all ten Chicago inserts | Processor state retained. Actual algorithm changes still reset the shared union. |
| Changed wet/time controls on active BPM Delay | Ring contents and write position retained. |
| Actual released-note tail | Ring energy 6.79517581 decays to 0.0732058702; output energy 4.01039989 instead of zero. |
| Eleven existing Chicago bridge/effect selectors | Exit 0, all selected checks passed. The unrelated package skip is not a package pass. |

The independent discovery probe also reproduced the tempo and frozen-tail
failures in the baseline WBL=1 whole-engine path. Before enabling the tagged
profile, the minimal runtime configurations passed the same 51/54 checks. One
initial fixture link referenced an optional preset API; replacing that fixture
reference with a direct binding observer restored the intended configuration
coverage. No production API was added to satisfy the fixture.

Enabling an insert now keeps that processor running while its dry oscillator is
idle. This is necessary for continuous tails and adds no processor state, but
host proof is not a device CPU-timing result. Native code/RAM/pool acceptance and
the combined independent review remain separate gates.

Logs beside this receipt retain the failed baseline and passing final profile.
