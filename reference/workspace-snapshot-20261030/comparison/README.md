# Exact archived version comparison (PARTIAL)

#734: `50866a1e43e3d2448daba4cc64a7a112c0a7cf42` (Git tree `8868d01c2c9d6e317a2df055aa30f7e2ebdb2e27`)
#739: `811d500e26061f6e68b9beb3de03d6cb50938ed5` (Git tree `82467c0e1217d16002b3bd7521b487ff422ce891`)
Registry: `docs/FEATURE-REGISTRY.md`, reviewed separately for feature-level classification.

This is exact-source presence only, NOT firmware feature acceptance. I=exact path in tree; R=blob in tree at another path only; D=path with different content; A=path absent.

| Archive | #734 I | #734 R | #734 D | #734 A | #739 I | #739 R | #739 D | #739 A |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| part1 | 9 | 3 | 44 | 3 | 9 | 3 | 44 | 3 |
| part2 | 3260 | 76 | 1186 | 173 | 3310 | 76 | 1136 | 173 |

All 4,754 archive variant rows are in the 20 comparison TSV packs. Classifications correspond to file-version inclusion only; a source file differing from the active head is not proof its functionality is missing. The 605 additional loose-folder records are metadata only, so no source-status claims are made for their unavailable bodies. #41.1–41.3 remain NOT BUILT as full owner behavior. Full host, Elite, Pro and hardware: NOT_RUN by this audit.
