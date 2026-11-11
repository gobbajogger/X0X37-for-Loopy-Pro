# Mixer effects page on the integrated build

Source: #688 (`wip/mixer-ux@7b2d1726`). This integration starts at `wip/one-build@cb5423c4aad7d9fc0c3e0a532c2aac080f3cfd21` and is related to #717 (independent locked-mixer page handling). Only compatible FX-page behaviour is transplanted. None of the older source snapshot's `experiments/` files, DSP substitutions, BLE queue changes or unrelated control changes are copied.

## Existing and restored controls

The FX page keeps the eight SELECT strips. Part SELECT momentarily applies low-pass; main-bus SELECT taps enter the reverb, delay or master mix page, while a held main SELECT activates momentary low-pass after the existing hold threshold. HOME + SELECT retains the existing preset-cycle gesture. The three existing named FX banks, ROOM, TIGHT and ECHO, and their bus-effect lookup remain in place. The caption labels now report the action for each strip and the active low-pass hold, not the stored momentary effect.

## Explicitly incomplete button map: #688 item 41

The approved complete FX-page button assignments for the other physical button rows are missing. In particular, there is no recovered owner-approved FX-specific mapping for REC and SOLO; the stored effect-bank assignments and HOME + SELECT cycling are **not** proof of a complete configurable FX-page button map. Do not infer new REC, SOLO or remaining-button effect actions. The on-device FX-page layout and controller LED targets also require actual integrated UI and device validation. This draft is not complete item-41 acceptance.

## Integration protection and checks

Preserve the newer base's BLE/SMC queue ordering, press/release ownership, project revision 13, original Chicago and Miami sound engines, and device-data protections. Test `sh tests/host/smc_fx_caption_test.sh`, then `sh tests/run_tests.sh` and SDK V1.2.1 Elite/Pro builds on the exact PR head. Native package, screen and hardware acceptance remain separate and require actual evidence.
