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

## Tip

Preferred product lineage: stream5 / #259 tip `6cf9be8c…`.
