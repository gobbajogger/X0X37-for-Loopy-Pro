#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Write STUB headers for the 10 private #684 inputs into a build directory.

The stubs contain synthetic values (ramps and constants). They contain no
original data. They exist only so the harness compiles. All audio, timing and
size numbers from a stub build are NOT VALID. The output directory is under
build/, which Git ignores. Do not copy these files into experiments/.
"""
import struct, sys
from pathlib import Path

BANNER = "/* STUB ONLY: synthetic values, no original data. Results are NOT VALID. */\n#define X0X_SYNTH684_STUB_INPUTS 1\n"

def arr(ctype, name, vals, dims=""):
    body = ",".join(str(v) for v in vals)
    return f"static const {ctype} {name}{dims}={{{body}}};\n"

def fbits(x): return struct.unpack("<I", struct.pack("<f", x))[0]
def ff(x): return f"{x!r}f"

def mapping_header(prefix, n_targets, n_params, n_base, factories, n_macro):
    m = f"typedef struct{{unsigned char type;float a,b;unsigned short lookup,length;unsigned short outer_count,canonical_count;unsigned short native_id;}} original_{prefix}_mapping;\n"
    rows = ",".join(f"{{0,0.0f,1.0f,0,1,0,128,{i}}}" for i in range(n_targets))
    m += f"static const original_{prefix}_mapping original_{prefix}_mappings[{n_targets}]={{{rows}}};\n"
    starts = [min(n_targets, (i * n_targets) // n_params) for i in range(n_params + 1)]
    m += arr("unsigned char", f"original_{prefix}_macro_start", starts, f"[{n_params+1}]")
    m += arr("float", f"original_{prefix}_lookup", [ff(i / 15) for i in range(16)], "[16]")
    m += arr("float", f"original_{prefix}_base", [ff(0.0)] * n_base, f"[{n_base}]")
    return m

def main(out):
    d = Path(out) / "experiments/original_synth"
    d.mkdir(parents=True, exist_ok=True)
    (Path(out) / "a/b/c").mkdir(parents=True, exist_ok=True)
    h = {}
    h["chicago_drive_private.h"] = arr("int16_t", "original_drive_table", [256 + 64 * i for i in range(128)], "[128]") + \
        arr("int16_t", "original_drive_gain_table", [128 * i for i in range(128)], "[128]")
    h["chicago_dynamics_private.h"] = arr("float", "original_chicago_eq_amplitude", [ff(1.0)] * 128, "[128]") + \
        arr("float", "original_chicago_eq_cos", [ff(0.5)] * 128, "[128]") + arr("float", "original_chicago_eq_sin", [ff(0.5)] * 128, "[128]")
    # Packed A/E: 16385 entries, width-0 second-order blocks (constant values).
    n = 16385; blocks = (n + 31) // 32
    def table(name, value):
        data = []
        for _ in range(blocks): data += list(struct.pack("<II", fbits(value), 0)) + [0]
        s = arr("uint8_t", f"{name}_data", data, f"[{len(data)}]")
        s += arr("uint16_t", f"{name}_offset", [9 * i for i in range(blocks)], f"[{blocks}]")
        s += f"static const original_lossless_table16 {name}={{{name}_data,{name}_offset,{len(data)},{n},{blocks}}};\n"
        return s
    h["chicago_packed_private.h"] = '#include "lossless_table.h"\n' + table("original_chicago_packed_a", 0.5) + table("original_chicago_packed_e", 0.0)
    p = mapping_header("chicago", 29, 22, 117, 81, 22)
    nbytes = (81 * 22 * 9 + 7) // 8 + 2
    p += arr("uint8_t", "original_chicago_factory_codes", [(i * 37) & 255 for i in range(nbytes)], f"[{nbytes}]")
    p += arr("float", "original_chicago_factory_dictionary", [ff((i % 128) / 127) for i in range(512)], "[512]")
    names = "".join(f"STUB C{i:02d}\\0" for i in range(81))
    p += f'static const char original_chicago_factory_names[]="{names}";\n'
    p += arr("unsigned short", "original_chicago_factory_name_offset", [8 * i for i in range(81)], "[81]")
    p += arr("float", "original_chicago_arp_base", [ff(36.0 + 12 * (i % 4)) for i in range(10)], "[10]")
    h["chicago_program_private.h"] = p
    h["chicago_tuning_private.h"] = arr("uint32_t", "original_chicago_scale2_bits", [fbits(0.0)] * 23, "[23]") + \
        arr("uint32_t", "original_chicago_scale3_bits", [fbits(0.0)] * 23, "[23]")
    h["miami_n513_private.h"] = arr("uint32_t", "original_miami_n513_bits", [fbits(1.0)] * 513, "[513]")
    m = mapping_header("miami", 24, 18, 109, 30, 18)
    m += "static const float original_miami_factory_macro[30][18]={" + ",".join(
        "{" + ",".join(ff(((f * 7 + i * 3) % 11) / 10) for i in range(18)) + "}" for f in range(30)) + "};\n"
    m += "static const char *const original_miami_factory_name[30]={" + ",".join(f'"STUB M{i:02d}"' for i in range(30)) + "};\n"
    h["miami_program_private.h"] = m
    h["modulation_private.h"] = "".join(arr("int32_t", nm, [256 * i for i in range(128)], "[128]") for nm in
                                        ("mod_linear", "mod_speed", "mod_feedback", "mod_ph_feedback", "mod_wet"))
    pitch = [int(1_000_000 * 2 ** (i / 12 / 4)) for i in range(128)]
    h["motion_tables_private.h"] = "#include <stdint.h>\n" + arr("int32_t", "original_pitch44100", pitch, "[128]") + "".join(
        arr("float", nm, [ff(0.25 + i / 512) for i in range(129)], "[129]") for nm in
        ("original_miami_a44100", "original_miami_b44100", "original_miami_r", "original_miami_cap"))
    h["miami_decimator_tables_private.h"] = arr("uint32_t", "miami_decimator_frequency", [1 + 512 * i for i in range(128)], "[128]") + \
        arr("int16_t", "miami_decimator_wet", [256 * i for i in range(128)], "[128]")
    for name, text in h.items():
        (d / name).write_text(BANNER + "#include <stdint.h>\n" + text)
    print(f"wrote {len(h)} STUB headers to {d}")

if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "build/synth684-harness/stubs")
