# LoopyX0X37 — source-exact native nightly build recovery (2026-10-24)

## Proven result and scope

Current source integration PR #103 head: `53c7d264c2b25d8f24dfe26a6ad5bbe9e527286e`; Git ROOT tree: `0ecdf6c2abdf2fbbe813781a62940805da4a28ae`; exact compiled `firmware/` subtree: `b3179883a83e9878b65a8a52385aa0210e866f05`.

**Actually executed with native JieLi clang4.0.1 Linux x86_64, actual source-exact firmware tree and exact pinned SDK V1.2.1 input hashes**: Elite and Pro target builds both exit 0; current package SHA256 Elite `6abb8656379d84bfbee6f31f59f11ae80b04b2af7c0e61be9fa88c46772f6810`, Pro `fbbc5cf92e303797bc7b361a4e510b8ac89e4710067f9b717c2d4bc17ee32ca0`. Code area 5892B free, static RAM 476B free, pool 8324B free vs required 8192B; 20 settings checked.

**QA is pending.** This is BUILD_ONLY and software package integrity evidence, not a complete latest-root QA, physical device PASS, or live nightly installable release. Source-only bootstrap support/test files are historical; do not call the historical root exact. Existing installer publicly remains empty. Firmware `nightly` protected ref and production main were not changed.

## Immutable Git source access without a shell clone

The connected GitHub read API can fetch exact immutable Git trees and plaintext Git blob contents even when this shell's direct network/DNS git clone fails. First read live PR HEAD SHA, then use the GitHub connector's documented GET `.../git/trees/{sha}?recursive=1` and `.../git/blobs/{blob_sha}` to fetch all changed compiled source inputs compared against an authorized prior source bootstrap. The current candidate changed 10 of 100 firmware blobs. Locally verify each replacement with `git hash-object <file>` equal to the exact API blob SHA. Run `git add -f firmware && git ls-tree $(git write-tree) firmware` and require the resulting subtree SHA to match the current GitHub tree exactly; also verify build.sh, tools/build.py, assets and controller shim inputs. Our current source recheck returned `b3179883a83e9878b65a8a52385aa0210e866f05` exactly; `assets/` and `ble/` tree SHAs matched live Git.

Do not copy old full-root evidence onto new code, reuse old firmware binaries for a newer firmware subtree, or infer source identity from a branch name. New PR HEAD requires a new immutable-source check and fresh build.

## Private exact SDK inputs instead of a false dependency blocker

The owner-provided SDK archive V1.2.0 does not satisfy the firmware's **pinned** AC79 V1.2.1 input checks. Do not substitute V1.2.0 or weaken the compiler's SDK validation. The exact three V1.2.1 placement files can be recovered privately from the OWNER'S earlier genuine checked `.fwsc`, using the repository's package parsing routines and read-only deinterleaving of outer package, flash headers and SFC app/config records. **Do not commit or publish those recovered vendor files.**

Validate exact SHA256 in source `tools/build.py`:

- `uboot.boot`: `4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4`
- `cfg_tool.bin`: `276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b`
- `cfg/eq_cfg_hw.bin`: `41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d`

The 3 out of 3 recovered files passed SHA256 exactly. Place under the private `AC79_SDK/cpu/wl82/tools/` tree, set `AC79_SDK` to its real absolute root; extract the owner-provided JieLi toolchain archive privately, set `JIELI_TOOLCHAIN` to the directory containing pi32v2. No proprietary SDK/controller binary belongs in public firmware or installer Git.

## Repeat current native DEV builds

Verify the real paths and exact Git SHA; the following environment placeholders are shell **variables already set to verified private paths** (do not copy the literal paths from a previous run):

```sh
BOARD=smk37 MODEL=elite X0X_DEV_PADCAL=1 bash build.sh --board smk37
BOARD=smk37 MODEL=pro   X0X_DEV_PADCAL=1 bash build.sh --board smk37
```

Capture real compiler exit, package SHA256/model/CRC, code area, static RAM, pool headroom, source SHA/tree/toolchain/SDK pins and negative tests. Never lower the 8192 B minimum free pool or confuse build PASS with hardware PASS.

## Correct staged downstream handoff

- Independent QA owns current source-matched whole-root regressions, including SMC host and BLE queue/payload/reconnect, sequencer, sample backup/restore/OTA/controller, wrong-model, truncation and cancellation. Historical test fixtures showed stale behavior and failed in several cases; do not call those tests QA PASS or assume they prove a new bug without corresponding current tests.
- The sole firmware Safe Merge role must review and protected-PR merge before `nightly` integration; native compiler is allowed **before** such merge and QA, but an installable picker is not.
- The NIGHTLY website is browser-source only. The separate firmware `.fwsc` has a documented GPL-3.0+Apache-2.0 source and notices distribution route (see current `LICENSING.md`), while JieLi `ctl.img` contains vendor libraries and is user-supplied locally, **never rehosted publicly**.
- Sole NIGHTLY Pages publisher may expose `DEV / HARDWARE TEST — DEVICE NOT TESTED` after QA, lawful immutable external hosting, exact HTTP package hash/headers, source/model/controller strategy, backup/OTA integrity and browser negative checks; prior owner hardware PASS is NOT required for NIGHTLY, but production needs matched hardware proof and explicit owner approval.

Evidence:
- Checked Git manifest: `evidence/classic-pr103-r10-native-build-20261024.json` on isolated verified evidence branch.
- Private artifact Library: `/LoopyX0X37/LoopyX0X37-PR103-R10-Native-Build-Evidence-20261024.zip` SHA256 `e50377effe0b95a9d54d63dbf0cc40e23313e115e297cc9035a4b5db07de6b3d` (logs/checks).
- Private owner-only Elite/Pro `.fwsc` Library: `/LoopyX0X37/LoopyX0X37-PR103-R10-PRIVATE-BUILD-ONLY-20261024.zip` SHA256 `0b88e2dce13560a94db06d79170d9ba3c4453b4258a1196e91120ea8a4345cdc` (not for flashing until QA).

This evidence establishes the actual executable build path. An inaccessible direct git remote, outdated SDK archive, nondevice environment, lack of nightly hosting or older failed QA is **not** a reason to stop independent source development or native BUILD_ONLY output; handle each distinct later safety/publication gate without faking success.