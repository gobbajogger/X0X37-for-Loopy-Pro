# X0X37 Pad and LED Index Path

## Physical pad placement

Status: CONFIRMED 2026-10-17, build 89f680e (x0x.bin sha256 87ce8dd35daef5f3), for the row order and the column order.
- The owner did the check with the PAD COLOURS orientation page (BEHAVIOR.md CLP-79). The owner confirmed that pad index 0 is top-left. The correction is 0 (export tools/padcal/pads-owner-20261017.txt).
- Limit: the orientation page lights one pad only. The check shows that pad index 0 is top-left. In the model of step 1 below, it thus excludes the corrections 8, 7 and 15.
- The check does not examine each of the other 15 pads. It does not examine the sensor order (SMK_PADMUX).

The SMK-37 pads make 2 rows of 8. The table shows the placement of the hardware pad index that the firmware assumes. You look from the top, and the keyboard is below the pads.

<!-- pad-placement:index -->
```
+----+----+----+----+----+----+----+----+
|  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |
+----+----+----+----+----+----+----+----+
|  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
+----+----+----+----+----+----+----+----+
```

- Pads 0-7 are the top row, left to right. Pads 8-15 are the bottom row, left to right.
- The LED of pad p is chain slot PADLED_SLOT[p] (firmware/src/lcd.c). The first table below gives the slots in the same layout.
- Mux A reads the top row. Mux B reads the bottom row. Pad p is at mux address a, with SMK_PADMUX[a] = p mod 8 (firmware/hal/smk_input.h). The second table below gives the addresses in the same layout.
- A padtest on a unit (2026-10-10) showed the rows (mux A = top, mux B = bottom). The owner check of 2026-10-17 (status line) showed the LED of pad index 0 at top-left.

<!-- pad-placement:led-slot -->
```
+----+----+----+----+----+----+----+----+
|  0 | 15 | 14 | 13 | 12 | 11 | 10 |  9 |
+----+----+----+----+----+----+----+----+
|  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |
+----+----+----+----+----+----+----+----+
```

<!-- pad-placement:mux-address -->
```
+----+----+----+----+----+----+----+----+
|  3 |  0 |  1 |  2 |  4 |  6 |  7 |  5 |
+----+----+----+----+----+----+----+----+
|  3 |  0 |  1 |  2 |  4 |  6 |  7 |  5 |
+----+----+----+----+----+----+----+----+
```

If the orientation page gives a correction N other than 0:
1. Read N. The lit pad that you confirmed is hardware pad N. Position i is on pad index i XOR N.
   - N = 8: the two rows change places.
   - N = 7: each row has the opposite direction (mirror).
   - N = 15: the two rows change places and each row has the opposite direction (180 degrees).
   - Other N: the error is unusual. Do not apply it as a fix. Make a padtest of all 16 pads first.
2. The correction stays in RAM only. It goes away at power off, and it does not change the sequencer, the clip grid or the notes.
3. To fix the placement, change the firmware pad map:
   - Sensor order: SMK_PADMUX in firmware/hal/smk_input.h.
   - LED order: PADLED_SLOT in firmware/src/lcd.c.
4. Change the three tables above to the new map. Run tests/pad_placement_doc_test.py and the pad tests.
5. Set the status to CONFIRMED, with the date and the build hash (x0x.bin sha256).

## Pad calibration entry names

docs/padcal-names.csv lists the 128 pad calibration entries: the velocity, the Akai colour, the name and the owner value (R, G, B of APC40_PAD_OWNER). The calibration mode shows the name of the selected entry (BEHAVIOR.md CLP-79).
- tools/apc_pad_colors.py makes the names from the Akai reference colour: a hue family and a shade. The groups 4-59 have 4 shades (LIGHT, FULL, DIM, DARK) and use the hue of their full shade.
- The Akai CSV has no colour names. These names are not Akai names and not Loopy Pro colour names.
- To make the list again, use `tools/apc_pad_colors.py --names-csv docs/padcal-names.csv`.

## Sequencer / grid mode (smk_pad_grid = 1)
- White-key index w maps directly to hardware pad w.
- Top row (pads 0-7) = steps 1-8.
- Bottom row (pads 8-15) = steps 9-16.
- SMK_PAD_OF_WHITE(w) = w
- SMK_WHITE_OF_PAD(p) = p

## Non-grid mode (notes low at bottom)
- White-key index w maps with a row swap.
- Low notes (white keys 0-7) go to bottom hardware pads 8-15.
- High notes (white keys 8-15) go to top hardware pads 0-7.
- SMK_PAD_OF_WHITE(w) = (w < 8 ? w+8 : w-8)
- SMK_WHITE_OF_PAD(p) = (p < 8 ? p+8 : p-8)

## LED chain (PADLED_SLOT)
Hardware pad index → chain slot:
{0, 15, 14, 13, 12, 11, 10, 9, 1, 2, 3, 4, 5, 6, 7, 8}

The top row goes in reverse order in the chain. The bottom row goes in sequence from slot 1.

## Clip-grid addition rule
A clip-grid mode that you add must set smk_pad_grid = 1 and use the direct index path.
It must not change the non-grid row-swap or the PADLED_SLOT table.

## APC40 mkII / Loopy Pro clip launcher (SMK-37 Pro and Elite)
- The firmware answers Universal MIDI Device Inquiry as Akai APC40 mkII (manufacturer `0x47`, product `0x29`) and answers the APC40 introduction handshake.
- APC logical clip pads use Note IDs `0x00..0x27` (40 pads, 8 columns × 5 rows). The firmware trims empty outer margins from the reported viewport while preserving all internal empty cells and their relative positions.
- An 8-wide layout shows one local 8×2 view (two rows) at a time. OCTAVE− and OCTAVE+ move this view down and up by two rows, within the latched clip area. A layout of 3 or 4 rows and 4 or fewer columns folds onto one 8×2 view. Source rows 0–1 (the lowest APC rows) map to physical pads 5–8 and 13–16. Source rows 2–3 map to pads 1–4 and 9–12.
- Loopy Pro owns clip state and color. Host feedback uses APC40's velocity-indexed 128-color palette on MIDI channels 0–15. For clip cells, the channel does not change the color. A zero velocity means an empty/off cell. APC feedback has no separate project-dimensions field. The firmware maps only the current 40-cell viewport.
- In clip mode, the order of the physical pad rows is upside down (vertical flip). Pad columns stay left-to-right. In clip mode, a short KNOB BANK tap does nothing. FADER BANK pins the SCALE / CHORD / ARP page, and `<` / `>` step its last field (BEHAVIOR.md CLP-76). The firmware sends no APC navigation messages. Loopy does not report clips outside its 8×5 APC box. Change the Loopy page to reach them. Holding KNOB BANK for more than 1 s toggles clip mode.
- The pad LED chain still uses `SMK_PAD_OF_WHITE(w)` with the mapped physical row. Normal-mode row swapping and the hardware `PADLED_SLOT` order do not change.
- Seq PLAY and Seq REC act only in clip mode. They send momentary APC40 PLAY (`0x5B`) and RECORD (`0x5D`) notes on press and release. Their LEDs follow Loopy's state feedback, so Play shows transport state and Rec shows recording/overdub state. Set Loopy Pro's active Record binding to the clips you want and choose the overdub behavior there.
- The firmware stores APC feedback as it arrives, also outside clip mode. It latches the layout 250 ms after the first feedback burst (at once on clip mode entry). The latched layout never shrinks, so clips that cue or play cannot shift pads. It grows only to complete a 4×4 fold. There is no staged feedback.
