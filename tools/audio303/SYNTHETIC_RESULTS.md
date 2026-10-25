# Synthetic diagnostic result

Reason: the host comparison math used to read private captures on import, so it
could not be checked without those files.

Purpose: record the import-safe feature, distance, and contour checks, including
the command, the exit, and the limits.

Effect for the player: none. Firmware DSP and the default sound stay as they are.
This note does not retune the instrument.

## Checked code

The numerical change and the tests are commit
`6fb4c6319be94f8169324254b5bfb1044c494e2d`.
Input read before that work was
`9b2e714677afa01dab21a18847f6e7ba96d0069a`.
`firmware/src/dsp/bass303.c` SHA-256 remains
`321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f`.

## Command

```sh
python3 tools/audio303/test_synthetic_diagnostics.py
```

Exit 0. 15 tests, all OK, about 1.65 seconds, on that commit, in this Cursor
cloud VM. `git diff --check` on the audio303 diff exited 0. `git diff --stat`
for `firmware` was empty. A separate comparison of valid finite buffers against
the previous formulas found 0 mismatches in 23 checks. GitHub Actions was not
used.

## What the tests showed

- A known harmonic stack at MIDI 48 stays inside 0.05 cent and 0.01 dB, and the
  pitch search does not report a boundary hit.
- Tones placed outside the ±1% search report the boundary flag and land on the
  near edge. The 0.99 and 1.01 limits are not the same distance in cents.
- Fewer than three reference partials at or above −40 dB score 999. Calibration
  rows then store JSON nulls for the rejected train fields.
- A zero or nonfinite buffer reports a null pitch. An all-zero buffer used to
  come back near 132.12 Hz.
- Changing gain changes RMS only. Relative harmonics, pitch, and spectral
  distance stay put.
- The note-48 grid choice does not move when the note-60 metrics change.
- Importing `analyze.py`, `contour.py`, or `slide_experiment.py` does not read
  private captures. `main()` is the private-reference entrypoint.

## Review

A separate reviewer read the source and re-ran the same test file on this tree.
Verdict: pass. No defect. Reviewer id
`bc-18f10bfe-f919-5218-bd69-b8f963112938`, source `internal`. That source is not
an owner-visible handoff, and the reviewer received no write ownership. This
package is complete on this branch, so no second writer was started.

## Limits

These are original synthetic tones. This run did not measure a Chicago plugin
or any private capture. Older held-out slide and spectral figures stay
historical. Wave, envelope, resonance, and accent equivalence remain unvalidated.

`plot.py` and `verify.py` still read private files on import. They were outside
this package. History `harmonic_distance_db` still stores the raw distance,
including 999, and does not use the calibration null pass. That matches the
previous script. `contour_hz` raises `ValueError` on a zero or nonfinite
window; `apparent_hz_at` reports that window with a null frequency.
`analyze.load` still asserts that private samples are finite. Default `main()`
still uses the checkout parent as the private workspace.

No firmware build, device flash, merge, NIGHTLY publication, or production
promotion was done. NIGHTLY still needs a fresh owner approval at the time of
that action. Bugbot review quota was not visible from this coding agent and
was not a block on the coding work.
