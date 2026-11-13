# X0X37 Pad and LED Index Path

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

Top row is reversed in the chain. Bottom row is sequential starting at slot 1.

## Clip-grid addition rule
When clip-grid mode is added, it must set smk_pad_grid = 1 and use the direct index path.
It must not change the non-grid row-swap or the PADLED_SLOT table.
