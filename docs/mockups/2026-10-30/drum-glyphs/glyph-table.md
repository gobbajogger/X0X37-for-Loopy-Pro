# Drum part-page glyphs (branch box/drum-glyphs)

Each glyph uses the 26x19 cell left of the readout. When idle, it shows the setting. On a hit, it follows the live voice envelope on the UI frame (909: the voice EG level / its peak since the hit; 808: the lane's block peak / the hit peak, falling no faster than -6 dB per 20 ms). When the voice ends, the idle glyph returns over 300 ms (the ink fades in or the wave regrows) and does not snap back. Only the changed cell is sent to the LCD.

| Control | What it does (engine) | Glyph | Adaptive / animated | Source of scaling |
|---|---|---|---|---|
| DECAY (909 BD/SD/toms/hats/cymbals), CP TAIL | amp decay | exponential tail to -60 dB; length 2+23*sqrt(ms/4000) px | length follows ms. Hit: the outline goes to a ghost, and the remaining tail (e high, (1-log2 e/-9.97)*len long) is filled with a white front edge, so it shrinks both ways | ms/s readout (engine_param_si) |
| 808 DECAY (unitless; toms in ms) | lane decay | same | same | pot / readout |
| 909 SD TONE | the snare noise decay (24 ms + 0.7 x value), not a filter | noisy tail | length follows the real ms; the noisy remaining tail shrinks | drum909.c noise_decay |
| TUNE (909 SD/toms/RS/CP Hz; 909 hats/cymbals st; all 808 st) | pitch | sine, 1+4*pos cycles, value tick; bipolar st controls have a centre mark at 0 st (pot 64) | cycles follow the value. Hit: the phase scrolls, amplitude is 1+6e (rings down), then regrows | Hz log axis; st linear around 0 |
| 909 BD TUNE / P.DPTH / PITCH | kick sweep tau, depth, pitch multiplier | the real pitch curve 0..100 ms on a 40..400 Hz log axis | the shape follows all three. Hit: a dot rides the curve at the real age, and the played part dims | drum909_bd_hz |
| ATTACK (909 BD/toms, 808 BD/CP) | click / transient | spike + body | height 2+15q. Hit: white flash under 30 ms, and the body scales by e | pot % |
| 808 BD TONE | low-pass 300*20^(v/127) Hz | flat-then-falling response with a knee | knee at the real Hz (100 Hz..10 kHz log). Hit: spectrum bars under the curve die faster at high frequencies (e^(1+2x/W)), so the spectrum tilts | drum808.c bd tone |
| 808 SD TONE | crossfade of the 173 Hz / 336 Hz shells | two peaks, heights cos t / sin t | the ratio follows the pot (piecewise tone01). Hit: peaks x(0.3+0.7e) | SD_F1/SD_F2 |
| SNAPPY (909 SD, 808 SD) | noise amount | noise-burst bars | height q. Hit: the burst flickers and fades with e | pot % |
| 808 SOUND (TOM/CONGA, RIM/CLAVE, CLAP/MARAC) | named switch | position pips | the selected pip grows with e | names list |
| KIT ACCENT | 909: velocity depth ("Veloc"); 808: accent | weak bar + accent bar, ">" marks | weak height 1-0.7q. Any hit in the kit pulses the bars up (0.6 -> 1.0) | pot value |

Files: pages/ (BEFORE|AFTER per page), gifs/ (cell, 6x, 40 ms frames), sheets/ (every 2nd frame with per-frame lit/white pixel counts), values/ (each control at 0/32/64/96/127), measure/ (per-frame numbers + summary.txt).
