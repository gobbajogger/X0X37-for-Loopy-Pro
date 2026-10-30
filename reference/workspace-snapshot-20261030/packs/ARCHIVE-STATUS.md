# Source snapshot audit status (PARTIAL)

The snapshot is a **read-only reference branch**. No firmware code, protected branch, binary or device was modified in the source project.

## Sources actually inspected

| Project/Library input | SHA-256 | Scope |
|---|---|---|
| x0x37-uncommitted-source-restoration-20261030.tar.gz | cb19904d3f08d52f20b72ec6234c4884573bdadd67dbec5be11cd4edf2c831dd | 9 worktrees; 59 retained text variants |
| x0x37-derived-source-restoration-20261030.tar.gz | 534139254108927223da16e31e2e6f8fe965d031fe429efc97bc303c417c28e6 | 4 worktrees; 4,695 retained text variants |
| dnbox-122-feature-reconciliation.xlsx | 2fde3919a9f1811ccf9cf9bc4064a86b7c66d84b2bc0d54d00e5c31237fd4770 | 122 rows exported without binary workbook |
| X0X37-SMC-two-peer-heap-owner-test-20261029.zip | cb8aaf197c522a9cf926125103b211531d5999c14563635cb5b5ba13f7c6335f | owner-test firmware binaries **excluded**; package checksum metadata retained |
| targeted-loose-source-inventory-605-versions.txt | 6cabff36cec8dd5be58fb34f860152aa5b9d8502419e2b3cad5b89548fb4bf3a | 605 metadata entries; 556 safe candidate aliases retained; source bodies missing |
| LoopyX0X37-text-source-restoration-20261030.tar.gz | 68c6810f3c439d469993fd5651dd6cee3347306d1654658d8bf8f4714f926d56 | separate project-wide source export, not proof the 605 loose-folder bodies were saved |

## Exact preservation method

- 931 unique source objects / 10,725,055 decoded UTF-8 bytes, verified against the original tar manifest SHA-256 and exact Git blob SHA-1.
- 4,754 original path/stage records: worktree, local branch name, original base SHA if recorded, original path, source hash, index/working/head stage.
- All source bodies are plain-text JSONL in 89 packs. The 20 path packs provide deterministic reconstruction. 
- The 605-version loose-folder list is **metadata only**: 556 eligible text candidate versions retained as references, 49 excluded as instruction/generated/unsafe-path metadata. Neither the 605 bodies nor complete local Git histories/stashes have been verified in the available data. **Do not call the workspace audit complete.**
- The archive's historical local branch name `master` is not evidence of a public GitHub ref or stable base commit.
- Do not revive `wobble.c`, bass303, migration logic or any protected device region from this archive. No isolated source version is automatically accepted firmware.

## Feature #41

The original `smc-live-toggle-review-r1-20261028-source-82117eb2` snapshot contains `smc_fx_map.h` and `smc_fx_map_tables.c` mapping **only eight momentary SELECT notes (24–31)**, with ROOM/TIGHT/ECHO saved SELECT-page maps. No full REC/SOLO/MUTE momentary map, 32 independent HOME/Back+button assignments or full 32-assignment per-page recall is demonstrated by this source copy.

- **41.1** SELECT/REC/SOLO/MUTE next to each fader play the chosen FX while held: **not built end-to-end** in #734; SELECT-only behavior exists, and separate #741 fixes one SELECT override in a draft.
- **41.2** HOME/Back+any one button cycles only that button's current-page effect: **not built** as 32 independent assignments; HOME+SELECT-only historical behavior exists.
- **41.3** switching FX pages recalls all 32 bus-suited assignments: **not built**; historical ROOM/TIGHT/ECHO sets cover 8 SELECT entries only.

Review separately against PR #739, PR #734 and `docs/FEATURE-REGISTRY.md`, pinned to those heads. #736, #740, #741 are existing partial review/implementation lanes; do not duplicate ports. Tests/builds/hardware: NOT_RUN in this archive stage.
