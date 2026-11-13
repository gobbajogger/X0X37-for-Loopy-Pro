# U1 execution evidence: six 240×240 design frames, 2026-11-13

**Status: design studies generated and inspected. NOT a captured firmware baseline or final side-by-side UI acceptance.** This is an execution checkpoint for PR #891, not a second tracker or runtime writer.

## Inputs / reproducibility

- GitHub design branch: `design/firmware-ux-overhaul-20261113`, design generator initially `872eb13211d25e06668b45d599679763fc6b9342`; after fixes `18f9730d2ad6faf18a28dd9ac44b047b95dc5ae7` and `8bf2a45539a6f8c1caa0b5e2cc9b01f14a92e0b2`.
- Local execution: `python tools/ux_prototypes/check_u1_frames.py` (calls generator). Pillow **12.3.0**, images native **240×240 RGB**.
- The local execution workspace has the PR generator source reproduced from the connected source text. Font-generator and Barlow font assets came from owner-provided **2026-10-23 project source bootstrap** (`source-at-7537a46.tar`); this is **NOT the pinned 2026-11-12 current firmware font build**, so letter metrics/rendering equivalence to exact current `94197fc8` cannot be claimed. The current GitHub `tools/gen_font.py` was inspected and uses the same Barlow role family, but no exact-byte font comparison was performed. Generated font fallback and alpha raster were those of the bootstrap source.
- Local font SHA256: Medium TTF `4998b693a190b76f2b11315844345a8546c1e2e6a320905507a9eed04edf8b48`; Bold `5bd6757e459a110da81b44532f0f43058e2dab2401688abca84f2fdbde193cbb`; SemiBold `bd299f4bc5b44d30be8d42e9bad3a5df7d66af1cd55d0ed72a8b8916360a1424`; historical `gen_font.py` `13e84d04c74cbcb0691632ab6483e85ba1d5d57969d237206680d200482ae4f2`.
- Generated PNG outputs are user-visible conversation artifacts and **not committed**, because the connected GitHub file API supports UTF-8 source only for this execution route. No SDK binaries or third-party font files were pushed.

## Actual local command / observed result

```text
python tools/ux_prototypes/check_u1_frames.py
U1 DESIGN STUDY PASS: 6 native 240x240 RGB frames, 96 text regions, 0 text bounds failures.
NOT PROOF: actual firmware UI frames, display hardware, audio or latency.
```

The initial five frames were rendered and visually inspected at native pixel content scaled 2× nearest-neighbour. Defects found from actual images: **PRESETS** title collided with right frame border; overly long footer values clipped by 60 px columns; selected FACTORY item displayed **USER** in the source footer. Fixed with short `BANK` header, shorter footer controls, and source `ROM` for factory selection. The sixth explicit inversion fixture was added. On final frames no text advance crosses its allocated region according to the focused check; image scaling and opacity are still display-dependent.

## Artifact manifest / visual review

| View, output (local) | SHA256 | Visual finding / design-only status |
|---|---|---|
| Home, `01-home.png` | `075f9fa0ed3ff715a330380fb74ba0459cc26eb7fd0030e3d2e2d9de39d843f9` | DnBOX exact spelling; part/engine visible; no text collision; preset label and default semantics illustrative, not hardware-proven |
| SEQ, `02-seq.png` | `9b26a92f3adfb0dce9550c0acc93b5f0702bcbc6f27c3dbc6e0568bc1c3d73b4` | Cmin7 plus all four actual note strings visible; active step distinguishable; chart is a *proposed* 16-step display |
| Held, `03-held.png` | `2ca6ab06b05b6d82aaeca01c7ebeb485712ff746c13e5d85d43cf58df3e36324` | Live key markers and separately coloured recorded-step markers; lacks octave/black-key differentiation, so not yet a complete piano-roll usability acceptance |
| Presets, `04-presets.png` | `018f0271e7749d87857f790cf6ad62af8ac3be381d11b36e92893c6ca2692c2d` | Header/footer clipping fixed; ROM now agrees with selected factory row; shown Juno factory entries are illustrative proposed content, NOT verified installed factory assets |
| Readout, `05-readout.png` | `9da926cc7855f8dbb51570cc51288b07342591b5642297984f2accf693fe539c` | Target `CHICAGO B CUTOFF`, value `87 / 127`; temp feedback overlaps SEQ step grid; whether it should cover playback is not accepted yet |
| Inversion, `06-inversion.png` | `7245c3bf150d6085652566ebdd2276d18c74ad21009b5cea0abe6afc29f185b5` | `C/E` with E4 G4 C5 visible; no forced note-only downgrade for a legitimate inversion |

## Side-by-side evidence availability (do not fabricate)

| Comparable state | Real pinned current firmware BEFORE | New U1 design study AFTER | Comparable pixel diff |
|---|---|---|---|
| HOME / Chicago | **NOT CAPTURED** | `01-home.png` | NOT_RUN |
| SEQ / Cmin7 | **NOT CAPTURED** | `02-seq.png` | NOT_RUN |
| Held Juno | **NOT CAPTURED** | `03-held.png` | NOT_RUN |
| Preset selection | **NOT CAPTURED** | `04-presets.png` | NOT_RUN |
| Parameter feedback | **NOT CAPTURED** | `05-readout.png` | NOT_RUN |
| Inversion C/E | **NOT CAPTURED** | `06-inversion.png` | NOT_RUN |

**Exact blocker:** the runtime workspace contains only a 2026-10-23 bootstrap source snapshot, not a verified complete pinned `94197fc8` host source corpus + matching generated fonts + baseline scenario input/PNG captures. The GitHub connector permits individual source reads but did not supply a complete local pinned tree, and it would be invalid to call the historical bootstrap's runtime output a 2026-11-12 current BEFORE frame. Existing `host/x0x_host.c` in repository can capture RGB565 framebuffers once supplied with exact pinned source and scenario. No firmware build is being requested for design alone.

## Independent modwheel coordination, not duplicate source ownership

[PR #893](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/893), candidate `bc54e1b2440759f5f9d1c550b31e8cd57281dca1`, reports **13 actual host-generated frames and 553 focused checks**, but those PNGs were not attached or directly accessible to this U1 execution. Its dual-target CC1 target/percentage readout and long UNASSIGNED behavior require comparing actual frame pixels and target names against the shared U1 feedback grammar. Its **flash free FAIL** (Elite 7676 B, Pro 7684 B, target >=8192) keeps it unreleasable; design signoff would not override this. No duplicate modwheel code or runtime writer is created.

## Next bounded check

Obtain an exact `94197fc8` host source/font/capture environment, run stable same-state baseline inputs in the real renderer, record RGB565-to-PNG hashes and visual side-by-side pairs; refine dense 37-key piano-roll labelling and SEQ overlay occlusion without editing firmware runtime. User/owner visual review follows real same-state captures; no implementation or device acceptance is implied.
