# Exact current new-project drum levels

**The final factory-project kick value is raw61, not the `LEVEL(94)` descriptor seed.** Current `engine_sound_defaults()` copies every parameter descriptor’s default, then applies an explicit factory mix: 909 BD Level becomes61 and 909 OH Level becomes60 (`engine.c:770–821`, especially `794,797`). These values are source-proven independently of any LCD pixels.

Source: `3c41ef4bedf9701f549ce2752b73306e2fab1eb8`, tree `fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d`. This is a read-only default-state disposition. No emulator, host, firmware, audio, device or render execution occurred; no candidate was changed.

## Final raw track values

The table follows each kit’s actual eleven-track display order. Parameter indices are zero-based in the corresponding `T_909` / `T_808` voice table.

| Track index | 909 voice | 909 Level parameter | DSP descriptor seed | **Final project raw Level** | 808 voice | 808 Level parameter | **Final project raw Level** |
|---|---|---:|---:|---:|---|---:|---:|
| 0 | BD | 3 | 94 | **61** | BD | 0 | **64** |
| 1 | SD | 4 | 58 | **58** | SD | 0 | **64** |
| 2 | LT | 2 | 52 | **52** | LT | 0 | **64** |
| 3 | MT | 2 | 52 | **52** | MT | 0 | **64** |
| 4 | HT | 2 | 52 | **52** | HT | 0 | **64** |
| 5 | RS | 0 | 103 | **103** | RS | 0 | **64** |
| 6 | CP | 0 | 122 | **122** | CP | 0 | **64** |
| 7 | CH | 1 | 89 | **89** | CB | 0 | **64** |
| 8 | OH | 1 | 80 | **60** | CY | 0 | **64** |
| 9 | CR | 1 | 47 | **47** | OH | 0 | **64** |
| 10 | RD | 1 | 42 | **42** | CH | 0 | **64** |

Copyable final arrays, in those exact orders:

```text
909: [61,58,52,52,52,103,122,89,60,47,42]
808: [64,64,64,64,64,64,64,64,64,64,64]
```

The full `engine_sound_defaults()` function contains no other per-voice Level override. Its other drum changes affect kick decay/drive, clap reverb and tom decay. The 808 kit Level is separately overridden to127, and part-mixer Levels are909=75 and808=112 (`engine.c:793–811`). Those kit/part values are **not** individual track Levels and must not replace the table.

Descriptor/lookup proof: `engine.c:469–501` resolves the drum metadata; `drum909.c:65,79–146,185–217` supplies the 909 Level seeds and index mappings. `drum808.c:1715–1716,1822–1905` supplies Level64 at parameter0 for each 808 track. The native 808 Level spec also uses raw64 across its sound variants (`1609,1623–1639`).

## Why DSP init is not the final project fixture

1. `engine_init()` initializes the two DSP engines (`engine.c:3242–3243`). Their native initialization and pot seeds are intermediate engine state, not the eventual stored project sound.
2. `project_defaults_for()` calls `engine_sound_defaults(&pr->sound)` and then initializes arrangement/pattern data; it does not renormalize track gains afterward (`project.c:106–117`).
3. At boot, `main_fm1.c:2410–2413` runs `engine_init`, tries `project_load()`, falls back to `project_defaults()` on failure, and then calls **`engine_apply_sound(&proj.sound)`**.
4. `engine_apply_sound()` applies the resolved project’s raw sound cells to the DSP (`engine.c:835–852`). The new-project factory overrides therefore supersede the DSP initialization seeds before normal audio startup at `main_fm1.c:2431`.

`drum909_init` seeds its own pot array from descriptor defaults (`drum909.c:525–527`); `drum808_init` seeds native sound pots (`drum808.c:2018–2027`). Neither establishes that a loaded/new project must display the native seed rather than the resolved project cell.

## Formatting and alignment limit

Current 909 Level gain is `1.35 * raw / 127`; 808 is `2 * raw / 127`. Both report `X0X_U_GAIN` (`drum909.c:65,253–264`; `drum808.c:1609,1924–1938`). `ui.c:1127–1129` formats that gain in dB, or OFF at zero. Thus the source-defined BD raw61 yields approximately −3.76dB and the source formatter’s −3.8dB treatment. This is an analytical consequence of source, **not** a raw value reverse-engineered from a captured display.

A successful load replaces the initial default sound with the stored sound object (`project.c:296–305,526–563`). Therefore **power-on does not guarantee factory values** for an existing project. This helper has not established the project bytes or setup history behind `elite-focused-04-mix-909`; its selected BD readout alone cannot prove the ten other tracks’ raw values, the historical package’s state, or exact before/after alignment.

R4 may use this table as a specifically labelled **current-source fresh-project fixture**. A comparison against an actual captured project still needs exact state/setup evidence; otherwise keep the controlled non-selected levels explicitly unqualified for state alignment. No pixel inference fills that gap.

The new current-source copy `current-source/firmware/src/app/project.c` was fetched through the connected GitHub read tool at the exact commit: Git blob **`cd61b3f155a8ae50d9ce227b2e3cfd7335585f2c`**, **36,887 bytes**, SHA-256 **`b64408cd073b2cf5cc6924450b4b8d255c61d11e607b8a77e19a2ca741626dd7`**. Other source identities remain in the existing intake receipts. Requested Sol6.1/ultra is retained; effective model/effort are not exposed.
