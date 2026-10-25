# Host gate amplitude result

Status: REJECTED. Held-out gate shape or velocity transfer did not clear the predetermined margin.

Reason: Chicago Gate stays nearly level while the source amp falls about 2.4 dB across the held note, and velocity 40 is quieter on Chicago than on the source.
Purpose: correct that held-note fall and that velocity step in the host renderer only.
Effect: a person playing the default instrument hears the same firmware sound. Passing the experimental flag makes held notes on this gate patch fall more slowly. It also applies one velocity exponent fitted at note 48. That exponent is not a general velocity match. Filter brightness, accent, and cutoff loudness are not corrected.

Coefficients, frozen on the training rows before held-out scoring: gain -12.67022449 dB, velocity exponent 0.53449463, slope 8.23 dB/s.
Command suffix: `--experimental-gate-model -12.67022449 0.53449463 8.23`.

The sample gain is `10**((gain_db + slope_db_per_s * t) / 20) * (velocity / 100)**beta`.
`t` is seconds since the note that opened the engine gate. A slide keeps that clock. A retrigger or a later repeated note starts it again. After the engine note-off the last value is held, including through the short release. The default renderer does not multiply by this gain.

Primary window: 50 ms bins 2 through 8, centers .125 s through .425 s, each contour normalized at bin 2.
Level: RMS from .1 s to .4 s. Velocity transfer is that level minus the same-note velocity 100 level at Chicago cutoff .6.
Absolute error uses the one training gain and does not renormalize each row.
A primary bin below linear RMS 1e-08 on either side is excluded.

## Aggregates

| Measure | Baseline dB | Model dB | Improvement dB |
| --- | ---: | ---: | ---: |
| Train shape RMSE | 1.5304 | 0.2267 | 1.3037 |
| Held-out shape RMSE | 1.5436 | 0.1097 | 1.4339 |
| Train velocity-40 transfer error | 4.2534 | 0.0000 | 4.2534 |
| Held-out velocity transfer RMSE | 1.3183 | 0.9024 | 0.4159 |
| Train absolute RMSE | 17.7122 | 7.9288 |  |
| Held-out absolute RMSE | 15.5885 | 7.1468 |  |
| Anchor absolute RMSE (cut .6, note 48, velocity 100) |  | 0.2601 |  |

Largest source-envelope agreement error against the stored reference renders: 0.0000 dB.

The pass margins, 0.5 dB of held-out shape RMSE and 0.5 dB of held-out velocity-transfer RMSE, were set before held-out scoring.
Held-out shape clears that margin. Held-out velocity transfer improves, and the improvement stays under the margin, so the joint result is rejected.

Held-out velocity transfer error after the frozen exponent (model minus Chicago, dB). Positive means the host is still louder than Chicago relative to velocity 100:

| Id | Baseline error | Model error |
| --- | ---: | ---: |
| vel80-r0 | 0.8714 | -0.1644 |
| vel127-r0 | -0.9032 | 0.2064 |
| velocity40-n60-r0 | 2.6430 | -1.6091 |
| velocity80-n60-r0 | 0.3239 | -0.7113 |
| velocity127-n60-r0 | -0.1561 | 0.9526 |

Note 48 velocities 80 and 127 move toward Chicago. Note 60 does not follow the same exponent: velocity 40 is overshot, and velocities 80 and 127 move farther away. The source velocity control also opens the filter, and that level effect depends on the note. An amplitude exponent fitted at note 48 cannot remove it.

## Rows

| Id | Role | Shape base | Shape model | Absolute model | Level model | Tail model | Second-note model | Pitch shift cents | Harmonic model |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| cut0.2-n48-r0 | train_shape | 1.4767 | 0.1015 | 33.9405 | -28.7303 | -45.7374 | null | 0.0000 | null |
| cut0.4-n48-r0 | train_shape | 1.4277 | 0.1426 | 3.8540 | -28.7303 | -45.7374 | null | 0.0000 | null |
| cut0.6-n48-r0 | train_shape | 1.5073 | 0.2558 | 0.2601 | -28.7303 | -45.7374 | null | 0.0000 | 5.3832 |
| cut0.8-n48-r0 | train_shape | 1.6070 | 0.3099 | 0.7292 | -28.7303 | -45.7374 | null | 0.0000 | 16.8332 |
| cut1.0-n48-r0 | train_shape | 1.6332 | 0.3237 | 0.8602 | -28.7303 | -45.7374 | null | 0.0000 | 17.5204 |
| vel40-r0 | train_velocity | 1.5081 | 0.2242 | 0.2288 | -44.6730 | -60.4802 | null | 0.0001 | 10.6722 |
| cut0.6-n60-r0 | heldout | 1.5415 | 0.0621 | 1.2908 | -32.5798 | -48.5357 | null | 0.0000 | 4.3823 |
| cut0.2-n60-r0 | heldout | 1.6072 | 0.1249 | 49.6867 | -32.5798 | -48.5357 | null | 0.0000 | 13.3456 |
| cut0.4-n60-r0 | heldout | 1.5458 | 0.0703 | 9.3234 | -32.5798 | -48.5357 | null | 0.0000 | null |
| cut0.8-n60-r0 | heldout | 1.5449 | 0.0663 | 2.5273 | -32.5798 | -48.5357 | null | 0.0000 | 23.1480 |
| cut1.0-n60-r0 | heldout | 1.5513 | 0.0718 | 2.7584 | -32.5798 | -48.5357 | null | 0.0000 | 25.6234 |
| vel80-r0 | heldout | 1.5031 | 0.2450 | 0.2713 | -32.7738 | -49.5516 | null | 0.0001 | 6.9329 |
| vel127-r0 | heldout | 1.5202 | 0.2699 | 0.3698 | -24.3700 | -41.2757 | null | 0.0000 | 3.6765 |
| velocity40-n60-r0 | heldout | 1.5379 | 0.0615 | 2.8993 | -50.1412 | -65.7928 | null | 0.0000 | 9.4411 |
| velocity80-n60-r0 | heldout | 1.5398 | 0.0617 | 2.0027 | -37.1711 | -52.8819 | null | 0.0000 | 5.7679 |
| velocity127-n60-r0 | heldout | 1.5442 | 0.0630 | 0.3381 | -27.4726 | -43.9699 | null | 0.0000 | 3.0688 |
| repeat-r0 | history | 1.5073 | 0.2558 | 0.2601 | -28.7303 | -45.7374 | 0.1598 | 0.0000 | 5.3832 |
| accent_repeat-r0 | history | 1.5073 | 0.2558 | 0.2601 | -28.7303 | -45.7374 | 3.3754 | 0.0000 | 5.3832 |
| overlap_glide0-r0 | history | 1.7764 | 0.3682 | 0.3507 | -29.4821 | -50.3861 | null | 0.3184 | 10.4453 |
| overlap_glide_mid-r0 | history | 2.0721 | 0.6499 | 0.5040 | -29.3453 | -45.4540 | null | 0.5752 | 9.4675 |
| overlap_retrigger-r0 | history | 0.5330 | 0.2493 | 0.2267 | -29.4854 | -44.9094 | null | -0.4724 | 6.6692 |
| repeat-n60-r0 | history | 1.5415 | 0.0621 | 1.2908 | -32.5798 | -48.5357 | 0.1351 | 0.0000 | 4.3823 |
| wave0.5-r0 | regression | 1.5583 | 0.1311 | 3.4783 | -29.2118 | -48.4919 | null | 0.0001 | 7.0437 |
| wave1.0-r0 | regression | 1.6352 | 0.1471 | 2.0867 | -29.2118 | -48.4919 | null | 0.0001 | 7.5575 |
| reso0.5-r0 | regression | 1.6356 | 0.4497 | 12.0288 | -39.0036 | -52.1908 | null | 0.0001 | 11.1059 |
| reso1.0-r0 | regression | 1.6447 | 0.3022 | 14.2981 | -38.3283 | -52.0178 | null | 0.0001 | 15.7506 |
| env_positive-r0 | regression | 2.4910 | 1.0265 | 1.1353 | -27.3489 | -45.5347 | null | 0.0030 | 8.1466 |
| env_amp-r0 | regression | 0.7679 | 0.9035 | 4.6135 | -29.6031 | -46.4156 | null | 0.0030 | 6.6906 |
| attack-r0 | regression | 1.0699 | 2.3540 | 5.2504 | -29.0038 | -45.9859 | null | -0.0323 | 5.7596 |

## Regressions and limits

Worst temporal change outside the fit is attack-r0: 1.2841 dB (positive means the model is farther from Chicago than the untouched render).
Clipped or non-finite model samples are counted per row in the JSON. The fit is rejected by the caller when any scored row is non-finite; clipping is reported rather than hidden.
Across-cutoff absolute error stays large because Chicago's low cutoff is much quieter and this fit does not add a cutoff curve.
The source accent path still raises the second repeated note. Chicago's accent-repeat trial uses the same events as the plain repeat, so that row remains a negative control.
Pitch shift is an FFT peak on .1–.4 s, model minus the untouched render. It is not the reference harmonic pitch fit. Scipy is not installed here, and this packet does not allow adding it. Harmonic distance uses a 12-partial least squares at the stored source f0, scored like the existing H2..H12 mask.

## What this does not say

The held-note shape improvement is limited to this gate patch. The velocity exponent does not clear the held-out margin. Wave, resonance, slide pitch, and accent stay separate. Raw Chicago audio stayed on the measurement machine. These numbers come from the approved derived envelopes plus fresh host renders of bass303.c.
