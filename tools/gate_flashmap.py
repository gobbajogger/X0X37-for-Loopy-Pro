#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check the SMK-37 flash map in firmware/hal/fm1_flash_map.h (read only).

The check finds overlaps between the regions of the BOARD_SMK37 map.
It also compares the map with the layout that the owner approved (a65 and later):
one Bluetooth block of 64 KiB at 0x8B000, the store at 0x9D000-0xAFFFF (with the owner tag at 0xAF000),
the 44 KiB VM spare at 0xB0000-0xBAFFF (not a sample slot), no piece 2 (FL_CTL2) and no FL_BOND.
The owner tag (ST_OWNER_SECTOR in firmware/src/storage.c) must be at 0xAF000, in the last sector of the store.
  tools/gate_flashmap.py firmware/hal/fm1_flash_map.h [firmware/src/storage.c]
The last line is "flashmap: PASS" or "flashmap: FAIL <reason>". The exit code is 0 or 1."""
import os
import re
import sys

src = open(sys.argv[1]).read()
start = src.index("#if BOARD_SMK37")
smk = src[start:src.index("#else", src.index("FL_GUARD", start))]
d = {k: int(v, 16) for k, v in re.findall(r"#define (FL_\w+)\s+0x([0-9A-Fa-f]+)u", smk)}
errors = []
regions = [("head", 0, d["FL_HEAD_HI"]), ("app code", 0x4120, d["FL_BLE_LO"]),
           ("Bluetooth block", d["FL_BLE_LO"], d["FL_BLE_HI"]), ("app slot end and cfg", d["FL_BLE_HI"], d["FL_APP_HI"]),
           ("store", d["FL_DATA_LO"], d["FL_DATA_HI"]), ("VM spare", d["FL_SMPV_LO"], d["FL_SMPV_HI"]),
           ("OTA", d["FL_OTA_LO"], d["FL_OTA_HI"]), ("BTIF and USRTRIM", d["FL_CAL_LO"], d["FL_CAL_HI"]),
           ("slots 1 and 2", d["FL_SMP_LO"], d["FL_SMP_HI"]), ("USR", d["FL_USRRES_LO"], d["FL_USRRES_HI"]),
           ("key_mac", d["FL_KEYMAC_LO"], 0x100000)]
if "FL_CTL2_LO" in d:
    regions += [("piece 2", d["FL_CTL2_LO"], d["FL_CTL2_HI"])]
    errors.append("piece 2 (FL_CTL2) exists")
if "FL_BOND_LO" in d:
    regions += [("bonds", d["FL_BOND_LO"], d["FL_BOND_HI"])]
    errors.append("FL_BOND exists")
regions.sort(key=lambda r: (r[1], r[2]))
for name, lo, hi in regions:
    print(f"  {lo:#08x}-{hi - 1:#08x} {(hi - lo) // 1024:4d} KiB  {name}")
for a, b in zip(regions, regions[1:]):
    if a[2] > b[1]:
        errors.append(f"overlap {a[0]} / {b[0]}")
expect = {"FL_BLE_LO": 0x8B000, "FL_BLE_HI": 0x9B000, "FL_DATA_LO": 0x9D000, "FL_DATA_HI": 0xB0000,
          "FL_SMPV_LO": 0xB0000, "FL_SMPV_HI": 0xBB000}
for k, v in expect.items():
    if d.get(k) != v:
        errors.append(f"{k} is {d.get(k, 0):#x}, expected {v:#x}")
st_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(sys.argv[1]), "..", "src", "storage.c")
m = re.search(r"#define ST_OWNER_SECTOR\s+0x([0-9A-Fa-f]+)u?", open(st_path).read()) if os.path.exists(st_path) else None
if not m:
    errors.append(f"no ST_OWNER_SECTOR in {st_path}")
else:
    owner = int(m.group(1), 16)
    print(f"owner tag (ST_OWNER_SECTOR): {owner:#x}")
    if owner != 0xAF000:
        errors.append(f"ST_OWNER_SECTOR is {owner:#x}, expected 0xaf000")
    if not (d["FL_DATA_LO"] <= owner and owner + 0x1000 <= d["FL_DATA_HI"]):
        errors.append(f"ST_OWNER_SECTOR {owner:#x} is not in the store")
print(f"VM spare: {d['FL_SMPV_LO']:#x}-{d['FL_SMPV_HI'] - 1:#x}, {(d['FL_SMPV_HI'] - d['FL_SMPV_LO']) // 1024} KiB")
print("flashmap: PASS" if not errors else "flashmap: FAIL " + "; ".join(errors))
sys.exit(1 if errors else 0)
