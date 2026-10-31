#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generated data for the 909 drum engine (firmware/src/dsp/drum909.c, fxbus.c).

Writes two headers (both flash-resident `static const` data):

  x0x_drum_samples.h  the hi-hat, ride and crash as IMA ADPCM, 44.1 kHz (assets/909/*.wav,
                      from ER-99 via 9W9, GPL-3.0). hh.wav is 24-bit: it is
                      rounded to 16 bits here (the 909's own cymbals are 6-bit),
                      then all three are encoded with tools/sampleio.py ima_encode
                      (predictor 0 / index 0, low nibble first; the format of the
                      BREAK loops). drum909.c decodes them as they play
                      (firmware/src/dsp/ima_adpcm.h). 4 bits a sample: 55,275 B of
                      flash instead of 221,098 B of int16.
  x0x_drum_tables.h   the tanh lookup used by every saturator (lossless IEEE
                      binary32 pack by float exponent: shared hi + packed low
                      bits, exact reconstruct via x0x_tanh_value), and 9W9's EXP
                      pot curves (er99_pots.h: min * (max/min)^(pot/127)) packed
                      the same way (x0x_pot_exp_value), so the device needs no
                      powf and gets the same values 9W9 computes with libm.

  tools/gen_drum_samples.py [build/gen/x0x_drum_samples.h]
The tables header is written next to the samples header.
"""
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sampleio as sio  # noqa: E402

SRC = Path(__file__).resolve().parents[1]
ASSETS = SRC / "assets" / "909"

# (C name, file). The closed hat plays the open hat's buffer (as in 9W9).
SAMPLES = [("x0x_smp_hh", "hh.wav"), ("x0x_smp_ride", "ride.wav"), ("x0x_smp_crash", "crash.wav")]

# 9W9's EXP pot ranges (er99_pots.h). One table per distinct (min, max).
EXP_RANGES = [
    ("BD_DECAY", 100.0, 4000.0), ("BD_PITCH", 0.43, 4.7), ("DRIVE", 0.85, 12.0),
    ("SD_TUNE", 130.0, 320.0), ("SD_TONE", 300.0, 4000.0),
    ("LT_TUNE", 50.0, 90.0), ("LT_DECAY", 80.0, 2600.0),
    ("MT_TUNE", 80.0, 125.0), ("MT_DECAY", 70.0, 2200.0),
    ("HT_TUNE", 110.0, 170.0), ("HT_DECAY", 60.0, 2000.0),
    ("RS_TUNE", 150.0, 300.0), ("CP_TUNE", 650.0, 1400.0), ("CP_TAIL", 120.0, 1000.0),
    ("OH_DECAY", 20.0, 1200.0), ("CH_DECAY", 15.0, 300.0), ("CY_DECAY", 100.0, 3000.0),
    ("SMP_PITCH", 0.25, 4.0), ("FX_HPF", 30.0, 800.0),
]

TANH_N = 1024          # points over [0, TANH_MAX]; linear interpolation
TANH_MAX = 8.0


def f32(x):
    return struct.unpack("<f", struct.pack("<f", x))[0]


def cfloat(x):
    s = repr(f32(x))
    if "e" not in s and "." not in s:
        s += ".0"
    return s + "f"


def read_wav(path):
    """first channel as a list of int16 (16-bit kept, 24-bit rounded)"""
    d = path.read_bytes()
    if d[:4] != b"RIFF" or d[8:12] != b"WAVE":
        raise SystemExit(f"{path}: not a WAV")
    i, ch, bits, data = 12, 1, 16, None
    while i + 8 <= len(d):
        cid, sz = d[i:i + 4], struct.unpack("<I", d[i + 4:i + 8])[0]
        if cid == b"fmt ":
            ch, bits = struct.unpack("<H", d[i + 10:i + 12])[0], struct.unpack("<H", d[i + 22:i + 24])[0]
        elif cid == b"data":
            data = d[i + 8:i + 8 + sz]
            break                                   # what 9W9's loader does: stop at data
        i += 8 + sz + (sz & 1)
    if data is None:
        raise SystemExit(f"{path}: no data chunk")
    bp = bits // 8 * ch
    out = []
    for k in range(len(data) // bp):
        p = data[k * bp:k * bp + bits // 8]
        if bits == 16:
            v = struct.unpack("<h", p)[0]
        elif bits == 24:
            v24 = int.from_bytes(p, "little", signed=True)
            v = max(-32768, min(32767, int(math.floor(v24 / 256.0 + 0.5))))
        else:
            raise SystemExit(f"{path}: {bits}-bit not handled")
        out.append(v)
    return out, bits


def arr(ctype, name, vals, fmt=str, per=12, shared=False):
    # shared: a table several units include, defined once (in the unit that defines
    # X0X_DRUM_TABLES_DEFINE, drum909.c) and declared extern in the rest. Static gave each unit its own
    # copy of the drum lookup payloads in the app's flash.
    if shared:
        return "\n".join([f"#ifdef X0X_DRUM_TABLES_DEFINE",
                          f"const {ctype} {name}[{len(vals)}] = {{"] +
                         ["    " + ", ".join(fmt(v) for v in vals[i:i + per]) + "," for i in range(0, len(vals), per)] +
                         ["};", "#else", f"extern const {ctype} {name}[{len(vals)}];", "#endif"])
    lines = [f"static const {ctype} {name}[{len(vals)}] = {{"]
    for i in range(0, len(vals), per):
        lines.append("    " + ", ".join(fmt(v) for v in vals[i:i + per]) + ",")
    lines.append("};")
    return "\n".join(lines)


def pot_exp(lo, hi, pot):
    """er99_pot_to_value for an EXP pot, in float as 9W9 evaluates it"""
    ratio = f32(f32(hi) / f32(lo))
    t = f32(pot / 127.0)
    return f32(f32(lo) * f32(ratio ** t))


def bits32(x):
    return struct.unpack("<I", struct.pack("<f", x))[0]


def pack_tanh_runs(tab):
    """Group tanh binary32 values by IEEE exponent; store shared hi + packed low bits.

    Same class as the EXP pot pack: exact reconstruction, no FP math in the accessor.
    Within one exponent the sign+exp field is constant, so the varying field is a
    contiguous low-bit mask (often the 23-bit mantissa). Returns (runs, words, raw_b, packed_b).
    """
    words_bits = [bits32(v) for v in tab]
    runs, payload = [], []
    start = 0
    for i in range(1, len(words_bits) + 1):
        if i < len(words_bits) and ((words_bits[i] >> 23) & 0xff) == ((words_bits[start] >> 23) & 0xff):
            continue
        chunk = words_bits[start:i]
        n = len(chunk)
        xor = 0
        for u in chunk:
            xor |= u ^ chunk[0]
        width = xor.bit_length()
        if width and xor != (1 << width) - 1:
            # Rare early bins: force full mantissa; sign+exp stay in base.
            width = 23
            xor = (1 << 23) - 1
        mask = xor
        base = chunk[0] & ~mask if width else chunk[0]
        assert all((u & ~mask) == base for u in chunk)
        offset = len(payload)
        if width:
            stream = sum((u & mask) << (k * width) for k, u in enumerate(chunk))
            payload.extend((stream >> bit) & 0xffffffff
                           for bit in range(0, n * width, 32))
        runs.append((base, offset, width, start + n))  # end exclusive
        start = i
    payload.append(0)  # final aligned high-word read; no out-of-bounds load
    raw_b = len(tab) * 4
    packed_b = len(runs) * 8 + len(runs) * 2 + len(payload) * 4  # run structs + ends + words
    return runs, payload, raw_b, packed_b


def main():
    out_s = Path(sys.argv[1]) if len(sys.argv) > 1 else SRC / "build" / "gen" / "x0x_drum_samples.h"
    out_t = out_s.parent / "x0x_drum_tables.h"
    out_s.parent.mkdir(parents=True, exist_ok=True)

    hdr = ["/* Generated by tools/gen_drum_samples.py -- do not edit.",
           " * Hi-hat, ride and crash from ER-99 (Matthew Cieplak) via 9W9, GPL-3.0.",
           " * Mono, 44.1 kHz, IMA ADPCM (4 bit, low nibble first, from predictor 0 / index 0;",
           " * firmware/src/dsp/ima_adpcm.h decodes it). _LEN is the sample count. */",
           "#pragma once", "#include <stdint.h>", ""]
    total = nbytes = 0
    for name, fn in SAMPLES:
        pcm, bits = read_wav(ASSETS / fn)
        data, _ = sio.ima_encode(pcm, 0)
        total += len(pcm)
        nbytes += len(data)
        hdr.append(f"/* {fn}: {len(pcm)} frames, source {bits}-bit, {len(data)} B of ADPCM */")
        hdr.append(f"#define {name.upper()}_LEN {len(pcm)}u")
        hdr.append(arr("uint8_t", name, list(data), per=24))
        hdr.append("")
    out_s.write_text("\n".join(hdr))

    tab = [f32(math.tanh(TANH_MAX * i / TANH_N)) for i in range(TANH_N + 2)]
    tanh_runs, tanh_words, tanh_raw_b, tanh_packed_b = pack_tanh_runs(tab)
    t = ["/* Generated by tools/gen_drum_samples.py -- do not edit. */",
         "#pragma once",
         "#include <stdint.h>", "",
         f"/* tanh(u) at u = i * {TANH_MAX} / {TANH_N}, i = 0..{TANH_N + 1} (one guard point).",
         f" * Lossless IEEE binary32 pack by float exponent: shared hi + packed low bits",
         f" * ({tanh_raw_b} B float table -> {tanh_packed_b} B runs/ends/words, exact reconstruct). */",
         f"#define X0X_TANH_N {TANH_N}",
         f"#define X0X_TANH_SCALE {cfloat(TANH_N / TANH_MAX)}",
         f"#define X0X_TANH_RUNS {len(tanh_runs)}",
         "typedef struct { uint32_t base; uint16_t offset; uint8_t width; uint8_t reserved; } x0x_tanh_run_t;",
         "#ifdef X0X_DRUM_TABLES_DEFINE",
         f"const x0x_tanh_run_t x0x_tanh_runs[X0X_TANH_RUNS] = {{"]
    t += [f"    {{0x{base:08x}u, {offset}, {width}, 0}},  /* end {end} */"
          for base, offset, width, end in tanh_runs]
    t += ["};",
          f"const uint16_t x0x_tanh_run_end[X0X_TANH_RUNS] = {{"
          + ", ".join(str(end) for *_, end in tanh_runs) + "};",
          "#else",
          "extern const x0x_tanh_run_t x0x_tanh_runs[X0X_TANH_RUNS];",
          "extern const uint16_t x0x_tanh_run_end[X0X_TANH_RUNS];",
          "#endif",
          arr("uint32_t", "x0x_tanh_words", tanh_words,
              lambda v: f"0x{v:08x}u", per=8, shared=True),
          "/* Exact binary32 table entry i (0 .. X0X_TANH_N+1). Hot-path inline. */",
          "static inline float x0x_tanh_value(unsigned i)",
          "{",
          "    unsigned r = 0, start = 0;",
          "    while (i >= x0x_tanh_run_end[r]) {",
          "        start = x0x_tanh_run_end[r];",
          "        ++r;",
          "    }",
          "    const x0x_tanh_run_t *run = &x0x_tanh_runs[r];",
          "    union { uint32_t u; float f; } result;",
          "    if (run->width == 0) {",
          "        result.u = run->base;",
          "        return result.f;",
          "    }",
          "    unsigned bit = (i - start) * (unsigned)run->width, shift = bit & 31u;",
          "    const uint32_t *p = &x0x_tanh_words[run->offset + (bit >> 5)];",
          "    uint32_t v = p[0] >> shift;",
          "    if (shift) v |= p[1] << (32u - shift);",
          "    result.u = run->base | (v & ((1u << run->width) - 1u));",
          "    return result.f;",
          "}", "",
          "/* 9W9 EXP pots (er99_pots.h): value = min * (max/min)^(pot/127), pot 0..127 */"]
    for i, (n, lo, hi) in enumerate(EXP_RANGES):
        t.append(f"#define X0X_EXP_{n} {i}   /* {lo:g} .. {hi:g} */")
    t.append(f"#define X0X_EXP_COUNT {len(EXP_RANGES)}")
    # All variable bits are contiguous low bits. Encode their exact binary32
    # representation, never recompute a curve in the runtime setter.
    words, rows = [], []
    for n, lo, hi in EXP_RANGES:
        vals = [bits32(pot_exp(lo, hi, p)) for p in range(128)]
        mask = 0
        for v in vals:
            mask |= v ^ vals[0]
        width = mask.bit_length()
        assert 0 < width < 32 and mask == (1 << width) - 1
        base = vals[0] & ~mask
        rows.append((base, len(words), width))
        stream = sum((v & mask) << (p * width) for p, v in enumerate(vals))
        words.extend((stream >> bit) & 0xffffffff
                     for bit in range(0, 128 * width, 32))
    words.append(0)  # final aligned high-word read; no out-of-bounds load
    t += ["typedef struct { uint32_t base; uint16_t offset; uint8_t width; uint8_t reserved; } x0x_exp_row_t;",
          "#ifdef X0X_DRUM_TABLES_DEFINE",
          "static const x0x_exp_row_t x0x_exp_rows[X0X_EXP_COUNT] = {"]
    t += [f"    {{0x{base:08x}u, {offset}, {width}, 0}},"
          for base, offset, width in rows]
    t += ["};", arr("uint32_t", "x0x_exp_words", words,
                     lambda v: f"0x{v:08x}u", per=8),
          "/* row < X0X_EXP_COUNT, pot < 128: callers retain their domain checks.",
          " * At most two aligned XIP payload loads plus row metadata; no cache or FP arithmetic. */",
          "__attribute__((noinline)) float x0x_pot_exp_value(unsigned row, unsigned pot)",
          "{",
          "    const x0x_exp_row_t *r = &x0x_exp_rows[row];",
          "    unsigned bit = pot * r->width, shift = bit & 31u;",
          "    const uint32_t *p = &x0x_exp_words[r->offset + (bit >> 5)];",
          "    uint32_t v = p[0] >> shift;",
          "    if (shift) v |= p[1] << (32u - shift);",
          "    union { uint32_t u; float f; } result;",
          "    result.u = r->base | (v & ((1u << r->width) - 1u));",
          "    return result.f;",
          "}", "#else",
          "float x0x_pot_exp_value(unsigned row, unsigned pot);", "#endif", ""]
    out_t.write_text("\n".join(t))
    print(f"gen_drum_samples: {total} frames ({nbytes} bytes of ADPCM, {total * 2} as int16) -> {out_s.name}; "
          f"tanh {TANH_N + 2} packed {tanh_raw_b}->{tanh_packed_b} B + {len(EXP_RANGES)} pot curves -> {out_t.name}")


if __name__ == "__main__":
    main()
