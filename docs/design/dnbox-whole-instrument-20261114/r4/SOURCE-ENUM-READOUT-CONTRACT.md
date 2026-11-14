# R4 edited enum/readout contract

Exact source `3c41ef4bedf9701f549ce2752b73306e2fab1eb8`; read-only source and lookup arithmetic, not runtime/test execution. Frozen candidate, Settings and CP1 unchanged.

| Engine/control | Edited canonical q values in clockwise order | Actual LCD labels in the same order |
|---|---|---|
| Chicago MODE id14 |0,64,127|DOWN, UP, UP/DN|
| Chicago RANGE id15 |0,64,127|1 OCT, 2 OCT, FULL|
| Chicago SPEED id16 |0,14,28,42,56,71,85,99,113,127|1/2, 1/4D, 1/2T, 1/4, 1/8D, 1/4T, 1/8, 1/8T, 1/16, 1/32|
| Miami RATE id13, WOBBLE row |0,18,36,54,73,91,109,127|1/2, 1/4, 1/8, 1/16, 1/3, 1/6, 1/12, 1/24|

MODE does **not** start with UP. Edited q0/64/127 → original_sound_control_value(q/127) → Chicago program lookup offsets2/3/4 → canonical native61 values1/3,2/3,0 → UI native mode indices1,2,0 → DOWN,UP,UP/DN. RANGE goes through lookup offsets5/6/7 → canonical native60 values0,1/5,2/5 → indices0,1,2 then+1 → ranges1,2,3. The LCD calls the third result FULL, not3 OCT or6 OCT.

SPEED readout is quantized from p->macro[16] with10 states. Its actual program targets also change: native54 quantized indices [0,4,8,8,10,12,12,14,14,15] out of15, native53 indices [9,9,8,9,9,8,9,8,9,9] out of9. Chicago program_routing reads these as arp_times [16,12,8,8,6,4,4,2,2,1] and arp_base_note [6,6,8,6,6,8,6,8,6,6]. These are source routing fields, not newly inferred audible timing or note claims. Miami RATE is read from p->macro[13] with8 states; its targets native48 indices [0,8,12,14,8,12,14,15] out of15 and native47 indices [9,9,9,9,8,8,8,8] out of9 come from its own lookup table.

For those controls, ui_original_states() supplies3/3/10/8. original_switch_turn() quantizes the **effective original value**, adds sign(direction), clamps the selector index, and writes `(index*127+(states-1)/2)/(states-1)` with integer division. One accepted turn moves one selector state; it does not add1 to the stored q byte and does not wrap. Multi-detent magnitude does not skip selector states in this helper. At a bound it retains that state. Base edits use original_sound_value() rather than only the rounded displayed byte; an existing step lock uses its q-derived value. Keep that distinction for the first edit.

Chicago FX TYPE id17 is a different type: raw choice0..11, max11, names table present. It does not use the q127 selector helper. turn_ref() advances/clamps raw choice by sign(e). Exact order:

`0 OFF; 1 CHO/FLG; 2 PHASER; 3 RING MOD; 4 COMPRESSOR; 5 DECIMATOR; 6 EQ; 7 ENSEM; 8 SH DLY; 9 BPM DL; 10 HALL; 11 GT REV`.

original_sound_control_value() explicitly remaps those raw choices to original macro indices [0,5,7,4,2,3,1,6,8,9,10,11] of the12-entry macro table. Program mapping/native113 yields FX type IDs [0,4,5,10,18,22,24,3,12,13,15,16]. Do not treat choice11 as11/127 or relabel these with an invented generic FX enum.

Miami TUNE id7 is continuous with an exact edited midpoint, not a named enum or percentage. original_sound_control_value() uses x=q/128 for q<64; q>=64 indexes the binary32 upper-value table corresponding to x=.5+(q−64)/126. q64 is exactly.5. Miami program mapping native80 is canonical(.25+.5*x,2001): a lookup-free linear mapping followed by rounding to k/2000. UI then computes signed hundredths `amount=quant(native80,2001)-1000` and formats sign/integer/two decimals. Native80's formatter domain can express−10.00..+10.00, but this edited macro only reaches−5.00..+5.00 because native80 is restricted to.25..75. Proved edited examples: q0→−5.00; q63→−0.08; q64→0.00; q65→+0.08; q127→+5.00. Source zero has no plus sign and source adds no unit suffix. Keep hundredths; do not invent a semitone/Hz suffix. Exact conversion uses the source binary32 table/FMA/canonical operations, rather than replacing the whole path with a generic percentage delta. Untouched factory-native values still require their original float path. Sources: original_sound.c70–77/135–169; miami_program_private.h13; miami_program.c14–42; ui_original_controls.h134–137; ui.c1103–1108.

Untouched factory gap: original_sound_value() restores original binary32 factory macro values while the edit bit is clear. original_text_view() reconstructs the original program and applies only actual edits/lock/motion previews. A rounded factory q byte alone is not proof of its initial native mode/range or nearest selector state near a boundary. These tables prove the **edited canonical states**; no per-factory81/30-bank native inventory or current audio/emulator execution is claimed here.

Exact source chain: ui_original_controls.h55–73 (state counts/quant); ui.c2875–2889 (one selector step),2976 (locks),3119–3130 (base/list),1084–1101 (LCD strings); original_sound.c67–84/160–169/274–298 (q conversion versus untouched factory values),5–15/28–29 (FX choices/remap); chicago_program.c17–37/68–76 (mapping/routing); chicago_program_private.h3/24–33 (MODE/RANGE/SPEED lookup metadata); miami_program.c14–42 and miami_program_private.h3/22–28 (Miami RATE mapping); ui_original_controls.h115–120/133–138 (native/macro readout fields). Retrieved exact program blobs are recorded in r4-enum-source-receipt.json.
