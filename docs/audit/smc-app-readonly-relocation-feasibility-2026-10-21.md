# App read-only data relocation: bounded feasibility

An exact 8 KiB set of immutable UI objects exists in the measured full-feature
application. Moving it could permit two more contiguous controller sectors
without moving any executable code outside the existing PC guard. This is
read-only feasibility, not a relocated link or firmware admission.

## Candidate objects

| Symbol | Bytes |
| --- | ---: |
| FONT_B, FONT_XS | 20 each |
| FONT_B_OFF, FONT_XS_OFF | 190 each |
| FONT_B_ADV, FONT_XS_ADV | 95 each |
| CORNER_OFF | 9 |
| FONT_B_DATA | 4323 |
| FONT_XS_DATA | 3250 |
| **Total** | **8192** |

Place objects in the order shown: pointer-containing font structs first,
16-bit offset arrays next, then byte arrays. This ordering does not require
alignment padding between objects. The final target linker must prove the
actual section size and relocation results; a sum of symbol sizes is not that
proof. Leave both FONT_*_BW arrays in primary app storage. All relocated
pointers must resolve to the intended final XIP addresses.

Sizes come from the previous full-feature `pattern-oz` target ELF, built from
816e0d3 with only the documented cold pattern unit flag change. ELF SHA-256:
`e71ab3fd075df94cbd8f47acae9b4e2dc399eccdf6332904105425afc940af2e`.
They can be inspected with the pinned pi32v2 `objdump -t`; generated font sizes
also appear in `build/gen/felucca_font.h`. No binary is committed here.

## Lifetime and explicit recovery change

`cv_text` reads the immutable glyph bitstreams. These are UI assets, not audio
render tables. FONT_XS and CORNER_OFF are used by normal UI/clip-key drawing.
However, FONT_B is also used once before normal UI operation:
`app/main_fm1.c:safe_main` renders `X0X SAFE MODE` with FONT_B.

Thus this candidate is conditional on replacing only that recovery heading's
font with retained FONT_S or FONT_M. The wording and recovery function remain,
but the visible rendering changes and requires review. No substitution has
been implemented. FONT_M remains for boot splash and calibration; FONT_S
remains for crash/error/recovery and update text. The other FONT_B references
found are normal application UI and clip-key drawing. Moving FONT_B without
this recovery correction is not safe.

## Admission and transaction requirements

The app must embed its own expected data address, length, version and CRC and
validate before the first normal UI dereference. The optional BLE controller
manifest cannot be its authority: these data are needed even with BLE off or a
missing controller. Missing/mixed/corrupt data must leave a USB recovery route
that does not dereference the moved assets. The application boot/update path
must retain all of its dependencies in primary storage.

Stage and verify new app data first, then install and admit the smaller app;
only then may expanded controller primary overwrite reclaimed old-app sectors.
The controller manifest is committed last. Store-owner migration, ordinary
store writes and rollback must preserve app data independently of BLE state.
New checks and descriptors themselves require measured app space.

Use a separate two-sector app-data region, not mixed app/controller ownership
within an erase sector. The exact region and loader protocol are not selected
by this note. An 8 KiB transfer from secondary storage to contiguous controller
primary provides **no increase in total flash capacity**. With the parent's
latest 103188-byte controller measurement, a 106496-byte overall controller
budget would retain only 3308 bytes before manifest/layout/admission overhead;
actual placement and app bridge may consume it. The latest controller text
90504 plus a 128-byte manifest fits a 92 KiB primary arithmetically (3576 bytes
remain), but not the earlier 84 KiB primary.

No default edits, profile startup, sample/guard changes, relocated build,
packaging or hardware tests were performed for this feasibility pass.
