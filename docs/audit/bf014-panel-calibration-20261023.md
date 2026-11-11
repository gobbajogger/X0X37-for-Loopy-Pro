# BF-014 — Ambiguous persisted panel calibration (LOW)

Status: HOST CHECKED / TARGET BUILD PENDING / DEVICE NOT TESTED / SOURCE ONLY.

## Reproduction and outcome

Production firmware repository gobbajogger/X0X37-for-Loopy-Pro; exact original nightly commit c9718ae833eb64a5191c350922593df8a36017c9; source tree 715fbd98d427c8ddd6b64d1b923e9d1fb8ae64ea.
Original firmware/src/app/panel.c Git blob cc5e72f814aaa71630a1e95db3a19bb55e126b35 independently matched by local git hash-object.
Repaired firmware/src/app/panel.c Git blob 60ca44f92557d8682227cdfe5d1ccd1e5c1bac54; tested regression blob 42c1edf65ab73d0370ddccd0db3cc01f23f76606.

panel_valid() checked magic, ID range and direction but not unique physical button/encoder IDs. CRC-valid/range-valid saved mappings assigning two actions to one button or encoder, or SMK37 reserved master encoder K1 to an app role, were accepted. panel_setup() already rejects these assignments. Such a saved map can break user controls after restart. Ordinary calibration corruption has NOT been observed; this is defensive hardening.

Expected: reject invalid persisted mapping and fall back to the stock panel. Original: accepts invalid map. The smallest correction tracks used buttons/encoders, reserves SMK master K1, and leaves distinct reordered mappings and reversed encoder direction legal. No panel storage format or samples are changed.

## Native executed evidence (Linux x86_64)

Original source was reconstructed from live Git blob and compared bit-for-bit using git hash-object. The repaired source+test blobs were also checked locally against the Git versions. Host harness stubs hardware reads and writes; does not replace target hardware or whole-application testing.

Negative baseline: GCC 14.2 and Clang 17, BOARD_SMK37=0: each exit 1, four deliberately failing assertions. BOARD_SMK37=1: each exit 1, five failing assertions. Errors are specifically duplicated physical mapping and reserved SMK master pot; invalid magic/ranges and valid defaults behaved as controls.
After repair: GCC and Clang, each BOARD_SMK37 0/1, optimization -O0/-O2, -Wall -Wextra -Werror and UBSan: 8/8 independently executed commands exited 0. BOARD_SMK37=0: 13 assertions each; BOARD_SMK37=1: 14 assertions each.
After repair: GCC and Clang both board variants, -O0 with AddressSanitizer+UBSan: 4/4 independently executed commands exited 0, same 13/14 assertions.

Negative baseline log SHA256: BOARD0 36ac3720b3d19adac4bb5ee901582c275ae74516771fb7cd95323009110032b4; BOARD1 0df7d354d961340b33371517513c2a0ccde4fe0e7a33b81b58d4d65379337f0a.
Post-fix log SHA256: BOARD0 3a7d95017bc12594178e5086e84fa010f1c7437316c5c15ccd7fe7525ba8d1f9; BOARD1 bf9039c5068ba83b6badaa2df9be1cb669408b962086e8e90189127fcecade8a. Full native commands/exits and log names are recorded in docs/scheduler-handoffs/bf014-panel-calibration.json.

SMK constants verified against live firmware/hal/smk_input.h: FM1_NBTN=18, FM1_NENC=8, SMK_ENC_MASTER=0. Both supported Elite/Pro share BOARD_SMK37 conditional behavior; host variants are not separate model builds.

## Scope, release and next owner

Not tested: full firmware/current-tree integration, Elite/Pro target builds, controller image, memory/pool/flash budgets, package model identity/CRC, sample backup/readback, actual hardware, firmware OTA, installer browser, production and NIGHTLY publication.
Independent QA must fetch the exact new PR HEAD and examine source+host test. Sole Safe Merge owns approved firmware nightly integration. Hourly Build owns genuine model-specific builds/gates. User owns physical device test; production requires exact tested bytes and approval.
Installed current Classic Build skill did not appear via skills__list; historical Library Project source was read, plus live Git owner/production docs/PRs. PR #78 docs/schema is unmerged, used as a non-authoritative format reference.
