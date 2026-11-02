# L12 Miami loudness: actual bounded host baseline

18 declared cases completed,36 process runs exit0. Instrumentation/pristine PCM identical for all18 (6,858,432 measured stereo frames); all8 corresponding A/B-slot pairs identical. No source gain edit, device operation, performance timing, PERF TEST or publication. The declared matrix preceded execution.

## Useful result
| Controlled case | Sample peak dBFS | Stereo RMS dBFS | Limiter occupancy |
|---|---:|---:|---:|
| Miami preset0 solo, Level100, neutral master |−25.770|−38.050|0%|
| Same, existing Level127 |−21.618|−33.898|0%|
| Preset15 solo, Level100 |−29.209|−37.840|0%|
| Same, Level127 |−25.057|−33.688|0%|
| Preset0 +909BD/SD, Level100, neutral master |−11.440|−28.963|0%|
| Same, Level127 |−10.598|−28.132|0%|
| Preset0 solo, factory master |−9.903|−22.873|0%|
| Preset0 +909BD/SD, factory master |−7.930|−24.898|99.993%|

The existing100→127 part-fader range delivers+4.152dB in these solos, without limiter gain reduction. This is an observed feasible existing-setting range for these exact neutral-master fixtures, NOT a recommended new permanent trim or proof for every preset/project. No additional gain value was tested.

Neutral-master worst pre-limiter peak in the declared matrix is0.590409 (preset0 mix, Level127), below the actual limiter gain-reduction threshold0.8, giving2.639dB threshold margin for a uniform scaling of THAT total waveform. Do not apply this as a Miami-only gain bound: its mix is a sum and cancellation/dynamics change with parts. Factory-master mix already reaches2.945866 before limiting; minimum limiter gain0.272003 (11.3085dB reduction). Thus no universal positive boost range is justified. More input can chiefly deepen master reduction, not increase useful Miami prominence. Factory-master solo gain differs substantially from the neutral-master solo. All final24-bit full-scale clip counts and observed nonfinite counts are0; NORMAL's output scaling and limiter mean this does not prove absence of earlier internal fixed-point clipping.

## Exact fixture and metrics
MATRIX.md was written before execution. Presets0/15, slotsA/B, solo or BDquarter+SDbackbeat, Level100/127; plus2 factory-master cases. MIDI48 velocity100,125BPM, sixteen quarter notes with half-quarter gate, then two quarter-note release tail:381024frames/case at44100Hz. Actual engine_bass_select_preset admission, real original engine/DSP/master. Complete sound_t unchanged after render; runtime bank1 and preset identity recorded. Nonselected synth receives no notes, not mute-as-isolation. Existing internal Miami preset FX retained; external sends0, sharedFX DIST/COMP0 and reverb/delay levels0; shared FX Volume44/127 retained. Master4096, NORMAL output, USB-full off. Neutral-master means master compressor/PUMP off, filter off, limiter on; it does not mean all internal synth effects bypassed. Factory-master cases retain factory compression/PUMP, still use these same externalFX controls. No user audio, private captures or device settings were used.

Stereo RMS =sqrt(mean(L²+R²)/2) over the complete active+tail fixture; sample peak=max absolute L/R; both normalized to24-bit full scale8388608. These are not LUFS, true peak, gated loudness, analog SPL or a device measurement. Observer records exact per-sample limiter gain<0.999999 occupancy, minimumgain, input-to-master and pre-limiter peak. No timing metrics collected. Highest peak is only across this finite matrix, not all30presets/allnotes/resonance/voices. No clipping instrumentation inside Miami's internal fixed-point FX conversion was added.

## Execution/provenance
Source8be16486e33f65b31c76710e1d825b6aa701c2c9, previously verified host closure; nonpublic build inputs are omitted from this packet; no redownload/rebuild of firmware or mutation of production source. Recovered source is sparse, so PR799 Git-snapshot adapter was not falsely passed. Its actual-bank admission approach and matched-slot principle are reused in a direct real-engine event harness; its19-row sweep/left-channel analyzer are not run or relabeled.

Commands: `python prepare.py`; `sh build.sh`; `python run_matrix.py`; `python report.py`. GCC14.2.0, -O1 -g -std=c99 -ffp-contract=off -fno-fast-math, ORIGINAL_USE_SOFT_FMA, MIAMI_FX_NO_HOST_FENV, X0X_HOST, BOARD_SMK37=1; the original build command was recorded in build.sh, which is omitted from this sanitized packet. Pristine and observer compile exit0; all36 case processes exit0; runner exit0. Observer is a disposable copy of master.c, observer.patch records exact differences. The first observer compile resolved the wrong same-named master.h through include ordering; corrected to the exact absolute DSP header, rebuilt once, then all PCM comparisons passed. Production headers untouched. Original archived result files recorded commands, exits, source/control/PCM hashes and actual metrics. This sanitized packet omits all 36 command arrays and build.sh; retained metrics and hashes are historical evidence, not staged reproduction instructions. Synthetic PCM retained locally only, not included in the evidence archive.

## Next owner action
Carry this as L12 listening-feedback evidence to the existing engine producer, not a new gain owner. Ask that owner to assess a bounded Miami-only post-engine part trim using this baseline, then compare intended louder balance under both neutral and factory master before picking a value. No silent VCA/filter/preset change, globalLOUD change, limiter weakening or automatic normalization. Current measurements establish room in selected neutral cases and significant factory-master limiting, not an approved production dB target. The user's exact preset/project/output settings remain unmeasured; PERF reset contamination is already proven separately and must not be introduced into this workflow. Native/hardware/listening acceptance remains pending.
