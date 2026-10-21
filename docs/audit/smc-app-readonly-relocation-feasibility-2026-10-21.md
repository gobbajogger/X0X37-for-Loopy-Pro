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

## Subsequent isolated target measurement

The parent authorized a disposable relocation measurement, not runtime adoption.
`tools/studies/smc_app_data/prepare.py` makes the exact study changes in an
explicit baseline archive export and suppresses package creation. Reproduce:

```sh
mkdir "$EXPORT"
git archive 816e0d3987bcb69be972cff2cd4df4469969f196 | tar -x -C "$EXPORT"
python3 tools/studies/smc_app_data/prepare.py "$EXPORT"
# Set JIELI_TOOLCHAIN and AC79_SDK to the established pinned local inputs.
(cd "$EXPORT" && X0X_DEV_PADCAL=1 CC=cc CXX=c++ python3 tools/build.py --board smk37 --model elite)
```

This fresh build uses baseline flags (not the earlier pattern-Oz change).
Measured results:

- `.app_ui`: exactly 8192 bytes, XIP `0x020A7000`, corresponding to physical
  flash `[0xAB000,0xAD000)`. No extra alignment bytes.
- Primary image: 540288 bytes versus baseline 548480: **8192 bytes smaller**.
- Primary free space under the original limit: 12384 bytes versus 4192.
- RAM `.data+.bss`: 97972 bytes, unchanged; pool 335740 bytes, unchanged.
- Existing target build checks passed. No installable package was produced.
- Extracted final linked `.app_ui` SHA-256:
  `32a8d485cc70adb8fd1739006e8f953748b8f562e7bef78b0b15416d4238f661`.

Linked font pointers were decoded from the section and compared to target ELF
symbols, in field order ADV, BW, OFF, DATA:

- FONT_B at `0x020A7000`: `0x020A71A4`, `0x020370D7`, `0x020A7028`, `0x020A726B`.
- FONT_XS at `0x020A7014`: `0x020A7203`, `0x02037078`, `0x020A70E6`, `0x020A834E`.

All eight pointers matched, including retained primary BW arrays. These are
this build's addresses, not a general wire contract. A final CRC must cover
linked data after its pointers are relocated.

This measured app still includes both custom BLE hosts and all musical/UI
features. It does not implement app-data admission, smaller-app-first update,
loader storage exclusions, or the new vendor app bridge. The generated primary
binary alone is intentionally incomplete and must not be installed. The safe
mode heading uses retained FONT_S with unchanged text; its visual appearance
has not been tested on hardware. Active profile-to-profile data replacement
also requires quiescing every reader, not merely a safe first-install order.

A byte comparison also passed for all seven relocated non-pointer arrays
(offsets, advances, corner offsets and both pixel streams) against the prior
full-feature pattern-Oz ELF's extracted text, indexed by its symbol addresses.
Only the font structs require address relocation; glyph contents are unchanged.
