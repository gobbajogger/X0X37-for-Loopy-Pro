# Original-control glyphs v2 (local box/new-glyphs, not pushed)
Every control is max=127 with a text readout. The type is taken from its readout (pref_value_of). Glyph box is 26x19 at x+3..x+28. Value text is centred at x+45 and starts at x+32 or later, so there is a 3 px gap.

| Engine | Control | What it does / type | Glyph | Why |
|---|---|---|---|---|
| CHI | GLIDE | portamento time, 0-100% | pitch step with a slope; the slope widens with the value | glide = pitch slews from one note to the next |
| CHI | CUTOFF | lowpass cutoff, 0-100% | lowpass response; the corner moves right | old filter_glyph / tone family, scaled into the cell |
| CHI | PEAK | resonance, 0-100% | lowpass at the live CUTOFF; the peak height follows PEAK | old filter_glyph (mode 1) |
| CHI | ENV | env-mod amount, bipolar -100..+100% | envelope curve around the midline, times the signed amount (flips below for negative values) | env curve x amount |
| CHI | BITE | drive / saturation | sine that soft-clips toward square | old drive_amount_glyph idea |
| CHI | GNAW | grit / harmonic fold | sine through a wavefolder; folds grow with the value | distinguishes it from BITE (clip) |
| CHI | ENV/GATE (AmpMod) | 2-state toggle ENV / GATE | ENV = AD/S/R shape, GATE = square gate | toggle -> 2 distinct pictures |
| CHI | LEVEL | output level | level wedge, filled up to the value | level |
| CHI | RANGE (ArpRng) | enum 1 OCT..FULL | octave stairs, one per state, lit up to the current one | octave span |
| CHI | TYPE (FxType) | enum 7 types (OFF..EQ) | one pip per type, current one lit | mode selector |
| MIA | LEVEL (carrier, xmod) | osc level | level wedge | level |
| MIA | SUB | sub-osc level | faint osc square + sub-octave square (half rate); its height follows the value | sub-octave wave |
| MIA | PITCH | +/-12 st, centre zero | centre needle | centre-zero |
| MIA | TUNE | +/-5.00, centre zero | centre needle | centre-zero |
| MIA | GLIDE | portamento | sloped pitch line | as Chicago |
| MIA | DEPTH (XmDep) | cross-mod (FM) depth | carrier wave with phase modulation; its distortion grows with the value | FM depth |
| MIA | CUTOFF | lowpass cutoff | lowpass response | filter family |
| MIA | CRUSH | bit/sample-rate crush | stair-stepped sine; fewer, coarser steps as the value rises | bitcrush |
| MIA | WOBBLE (WobDep) | LFO depth | LFO sine, amplitude = value | LFO amount |
| MIA | RATE (WobRat) | LFO rate 1/2..1/24 | LFO sine, cycles = 1..5 by the value | LFO frequency |
| MIA | OUTPUT | output level | level wedge | level |
| MIA | STEREO | stereo width | L/R bars spreading from the centre | L/R spread |

Unchanged: CHI WAVE, MIA WAVE/XMOD WAVE/SHAPE (wave glyphs), CHI ARP ON/MODE/SPEED (power/arrow/clock), CHI A/D/S/R (ADSR graph), CHI FX EDIT 1/2 (plain value; their meaning depends on TYPE).

7-bar scale (last resort): used by no current control. It is kept only as the fallback branch.
