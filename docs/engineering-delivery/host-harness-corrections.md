# Maintained host harness/API compatibility corrections

Five default-registry cases failed before execution on both d633 and8333 using
identical compiler/reference inputs: font-pack, drum909, drum808, bass303 and
validation-inputs. This unit restores current source API/dependency/fixture
contracts without removing warnings, weakening assertions or tuning DSP.

- The font fixture uses the existing host simulator's scoped attribute elision
  for the included gfx renderer, retaining every raw-versus-packed pixel check.
- The909 oracle calls the real losslessly packed pot lookup and both drum
  harnesses link the real shared parameter helpers. Existing reference/behavior
  thresholds are retained; actual909 and436808 cases execute and pass.
- The303 comparison isolates every actual public variant symbol, uses explicit
  independent23-parameter stock extension values and current waveform endpoints.
  The existing reference-fit thresholds remain unchanged. Its baseline envelope
  amplitude mismatch is DEFERRED_REFERENCE_FIT_NOTPASS under owner direction;
  no fit tuning or false green result is implied.
- The declared installed-files manifest uses resolved paths and portable relpath
  conversion, retaining outside-root console scripts and existing provenance,
  anchor and symlink checks on Python3.9/3.11.
- Only the unused local loop-index declaration in clear_audio_state is guarded
  for OS>=2, matching all its existing uses. No arithmetic, loop or runtime DSP
  statement changes. Default native preprocessed/object identity is a separate
  required proof before this unit is accepted.

Raw full8333 results remain259PASS/16FAIL+2optionalAU-not-run. Component repairs,
independent baseline controls and case-level closure must be reported truthfully;
this document is not full-regression or native/device qualification.
