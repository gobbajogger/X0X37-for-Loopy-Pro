# Velocity→cutoff host measurement

Status: **MEASUREMENT_COMPLETE_NO_GATE_FILTER_FIT**.

Chicago Gate AU brightness is flat across velocity while host bass303 already darkens/brightens. This Gate packet cannot calibrate a velocity→filter correction toward Chicago. Do not import the rejected joint gain model. Request non-Gate Chicago filter-velocity derived metrics before changing the firmware law.

## Why this exists

Owner: velocity in 303 works for level but should map lower velocity to lower
cutoff and higher velocity to higher cutoff, separate from volume/accent and
the persistent cutoff base. The rejected joint gain tip
`a33c2de208c227beab7203fbb2de9e244052eff6` is not imported.

## Commands

```sh
cc -std=c99 -O2 -ffp-contract=off -Ifirmware/src -Ifirmware/src/dsp \
  tools/audio303/render.c firmware/src/dsp/bass303.c -lm -o /tmp/production303
python3 tools/audio303/velocity_cutoff.py
python3 tools/audio303/test_velocity_cutoff.py
```

bass303.c SHA-256 actual `321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f`
(expected `321bcc141e6f234d098eb5180f47eae1afc3b6195fd08ecb5dbe54ccb8cd0a1f`).

## Train (note 48, cutoff pot 64 / Chicago .6 Gate patch)

| Measure | Value |
| --- | ---: |
| AU brightness transfer vel40−vel100 (dB) | 0.0127 |
| Host brightness transfer vel40−vel100 (dB) | -3.6740 |
| AU RMS transfer vel40−vel100 (dB) | -15.9427 |
| Host RMS transfer vel40−vel100 (dB) | -11.6893 |
| AU Gate brightness flat (≤0.05 dB) | True |
| Host velocity→cutoff active (≥0.5 dB) | True |
| Host matches stored DSP harmonics | True |

## Held-out brightness transfers (host)

| Id | dB vs same-note vel100 |
| --- | ---: |
| vel80-r0 | -1.1550 |
| vel127-r0 | 1.5093 |
| velocity40-n60-r0 | -5.1839 |
| velocity80-n60-r0 | -1.5147 |
| velocity127-n60-r0 | 1.7835 |

## Firmware law (state only)

 Neutral velocity 100: gain 1, cutoff multiplier 1.
 `velocity_cutoff = 2**((v-100)*1.5/100)`; `velocity_gain = v/100`.
 Pot cutoff is not written by note-on.

| Velocity | gain | cutoff mult |
| ---: | ---: | ---: |
| 40 | 0.400000 | 0.535887 |
| 80 | 0.800000 | 0.812252 |
| 100 | 1.000000 | 1.000000 |
| 127 | 1.270000 | 1.324089 |

## What this does not say

- Not Chicago acoustic equivalence.
- Not a native/AU Mac result.
- Not authorization to change the 1.5 cutoff exponent without non-Gate metrics.
- Not integration of the rejected joint amplitude model.

JSON: `tools/audio303/data/velocity-cutoff-results.json`.
