#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Optional break-loop pack.

The default firmware image does not contain the two factory loops. This module
builds the pack the installer writes into the free tail of the app code area,
and the empty break-bank header the default build compiles.

A pack is the whole IMA ADPCM of each loop, from the first sample, predictor 0.
It does not share or cut a prefix. Reverse plays the same bytes the loop always used.

  tools/break_pack.py --from-export assets/factory-breaks --js web/factory_break_pack.js
"""
import argparse
import base64
import struct
import zlib
from pathlib import Path

MAGIC = b"X0XBRKPK"
VERSION = 1
RATE = 44100
MAX_LOOPS = 8

EMPTY_HEADER = """/* Default firmware: no built-in break loops in the app image.
 * The factory loops are assets/factory-breaks/. The installer can put that pack,
 * or your own loops, into the free tail of this image. X0X_FACTORY_BREAKS=1 puts
 * the factory loops back into the image. X0X_BREAK_BANK=DIR uses another bank.
 */
#pragma once
#include <stdint.h>
#define X0X_NBREAKS 0
#define X0X_BREAK_RATE 44100u
typedef struct { const char *name; const uint8_t *adpcm; uint32_t n; uint8_t bars; } x0x_break_t;
static const x0x_break_t X0X_BREAKS[1] = {{0, 0, 0, 0}};
"""


def write_empty_header(path):
    Path(path).write_text(EMPTY_HEADER)


def _name(s):
    raw = s.encode("ascii", "replace")[:7]
    return raw + b"\0" * (8 - len(raw))


def encode_pack(loops, rate=RATE):
    """loops: [{name, adpcm: bytes, nsamples, bars}]. Full ADPCM, no prefix splice."""
    if not 1 <= len(loops) <= MAX_LOOPS:
        raise ValueError(f"a pack holds 1 to {MAX_LOOPS} loops")
    if rate not in (44100, 22050):
        raise ValueError("rate must be 44100 or 22050")
    body = bytearray()
    for loop in loops:
        adpcm = bytes(loop["adpcm"])
        ns = int(loop["nsamples"])
        bars = int(loop["bars"])
        if ns < 64 or (ns + 1) // 2 != len(adpcm):
            raise ValueError(f"{loop['name']}: ADPCM length is not the whole loop")
        if bars not in (1, 2, 4, 8):
            raise ValueError(f"{loop['name']}: bars must be 1, 2, 4 or 8")
        body += _name(loop["name"])
        body += struct.pack("<IB3xI", ns, bars, len(adpcm))
        body += adpcm
    crc = zlib.crc32(body) & 0xFFFFFFFF
    return bytes(MAGIC) + struct.pack("<HHII", VERSION, len(loops), rate, crc) + body


def parse_pack_prefix(buf):
    """Return (rate, loops, end) or None. end is the first byte after the pack."""
    buf = bytes(buf)
    if len(buf) < 20 or buf[:8] != MAGIC:
        return None
    version, count, rate, crc = struct.unpack_from("<HHII", buf, 8)
    if version != VERSION or not 1 <= count <= MAX_LOOPS or rate not in (44100, 22050):
        return None
    pos, loops = 20, []
    for _ in range(count):
        if pos + 20 > len(buf):
            return None
        name = buf[pos:pos + 8].split(b"\0", 1)[0].decode("ascii", "replace")
        ns, bars, nbytes = struct.unpack_from("<IB3xI", buf, pos + 8)
        pos += 20
        if nbytes != (ns + 1) // 2 or pos + nbytes > len(buf) or bars not in (1, 2, 4, 8) or ns < 64:
            return None
        loops.append({"name": name, "nsamples": ns, "bars": bars, "adpcm": buf[pos:pos + nbytes]})
        pos += nbytes
    if zlib.crc32(buf[20:pos]) & 0xFFFFFFFF != crc:
        return None
    return rate, loops, pos


def parse_pack(buf):
    """Return (rate, loops) or None. The buffer is one pack and nothing after it."""
    got = parse_pack_prefix(buf)
    if not got or got[2] != len(buf):
        return None
    return got[0], got[1]


def _valid_at(app, at, code_max):
    if at < 0 or at + 20 > code_max or bytes(app[at:at + 8]) != MAGIC:
        return False
    parsed = parse_pack_prefix(app[at:code_max])
    if not parsed:
        return False
    end = at + parsed[2]
    return end <= code_max and all(b == 0xFF for b in app[end:code_max])


def place_pack(app, pack, code_max=None):
    """Write pack into the free tail of app (a bytearray). code_max defaults to len(app).

    The image ends with a 0x00 tail mark. The pack starts on the next 16-byte
    boundary after that mark and must end at or before code_max. Bytes at
    code_max and after (the Bluetooth block on the SMK-37) are not written.
    """
    app = bytearray(app)
    code_max = len(app) if code_max is None else code_max
    if code_max > len(app):
        raise ValueError("code_max past the buffer")
    pack = bytes(pack)
    if parse_pack(pack) is None:
        raise ValueError("not a break pack")
    at = None
    o = (code_max - 20) & ~15
    while o >= 0:
        if _valid_at(app, o, code_max):
            at = o
            break
        if o == 0:
            break
        o -= 16
    if at is None:
        i = code_max - 1
        while i > 0 and app[i] == 0xFF:
            i -= 1
        at = (i + 1 + 15) & ~15
    if at + len(pack) > code_max:
        raise ValueError(f"loop pack needs {len(pack)} bytes, {code_max - at} free")
    app[at:code_max] = b"\xFF" * (code_max - at)
    app[at:at + len(pack)] = pack
    return app


def loops_from_export(directory):
    directory = Path(directory)
    loops = []
    for line in (directory / "pack.txt").read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        file, ns, bars, name = line.split(None, 3)
        adpcm = (directory / file).read_bytes()
        loops.append({"name": name, "adpcm": adpcm, "nsamples": int(ns), "bars": int(bars)})
    return loops


def js_module(pack):
    b64 = base64.b64encode(pack).decode("ascii")
    chunks = " +\n".join(f'  "{b64[i:i + 96]}"' for i in range(0, len(b64), 96))
    return (
        "// SPDX-License-Identifier: GPL-3.0-only\n"
        "// Factory break loops (909 GR, 909 FL), full IMA ADPCM. Generated by tools/break_pack.py.\n"
        "// The default firmware does not contain these bytes. The installer can write them back.\n"
        "export const FACTORY_BREAK_PACK = Uint8Array.from(atob(\n"
        + chunks
        + "\n), (c) => c.charCodeAt(0));\n"
    )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--from-export", type=Path)
    ap.add_argument("--js", type=Path)
    ap.add_argument("--bin", type=Path)
    a = ap.parse_args()
    if not a.from_export:
        raise SystemExit("need --from-export")
    pack = encode_pack(loops_from_export(a.from_export))
    parsed = parse_pack(pack)
    if parsed is None:
        raise SystemExit("pack did not parse")
    print(f"break pack: {len(parsed[1])} loops, {len(pack)} bytes")
    if a.bin:
        a.bin.write_bytes(pack)
    if a.js:
        a.js.write_text(js_module(pack))
        print(f"wrote {a.js}")


if __name__ == "__main__":
    main()
