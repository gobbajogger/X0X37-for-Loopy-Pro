# Chicago / Miami factory banks (private/dev)

Faithful-303 factory defaults for **Chicago (Acid)** and **Miami (Wobble)**.
These are not an “upgrade” or “better 303” brand. WBL1 remains a separate
alternate backend; the Miami factory bank is its own brand bank.

## Counts

| Brand   | Factory presets | Source on owner Mac |
| ------- | --------------: | ------------------- |
| Chicago |              81 | KORG Gadget `ChicagoLibrary.bundle` (`Factory2.prog` + NKS) |
| Miami   |              30 | KORG Gadget `MiamiLibrary.bundle` (NKS) |

## Product rules

- **Factory by default** — banks ship built-in (`firmware/src/app/factory_banks.*`); no mandatory user import.
- **User import allowed** — `user_bank_t` stays writable and must not overwrite factory ROM.
- **Private/dev** — owner-paid personal-use packs. Owner strips these before public redistribution. No strip automation in-tree unless asked.

## Files

- `chicago.json` / `miami.json` — full float parameter banks + provenance
- `manifest.json` — counts and framing
- Regenerator: `tools/import_chicago_miami_factory_banks.py` (reads the Mac Gadget packs)

## Chicago Arp pots (ids 13–16)

Factory bytes decode with `chi_arp_decode_factory` / `chi_arp_apply_factory_voice`
(`firmware/src/app/chicago_arp.h`). Product path: PART → CHI ARP page drives live
freeplay/REC held-note arpeggiation on native303. Clip-key ARP (`ck_arp*`) stays
MIDI-to-Loopy only. Pattern step playback is unchanged when Switch is Off (default).

## Tip

Preferred product lineage: stream5 after #281 (Bite/amp ADSR).
