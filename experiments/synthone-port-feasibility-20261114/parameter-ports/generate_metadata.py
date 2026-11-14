#!/usr/bin/env python3
"""Generate only pinned public parameter metadata and immutable compact maps."""
import hashlib
import json
from pathlib import Path
import re
import sys
from types import ModuleType

ROOT = Path(__file__).resolve().parent
if len(sys.argv) != 3:
    raise SystemExit("usage: python3 generate_metadata.py SYNTHONE_SOURCE_ROOT OUTPUT_DIRECTORY")
SOURCE = Path(sys.argv[1]).resolve()
OUTPUT = Path(sys.argv[2])
checker_path = ROOT / "verify_inputs.py"
checker = ModuleType("checked_inputs")
checker.__file__ = str(checker_path)
exec(compile(checker_path.read_bytes(), str(checker_path), "exec", dont_inherit=True), checker.__dict__)
checker.verify(SOURCE)
HEADER = SOURCE / "AudioKitSynthOne/DSP/Kernel/S1DSPKernel.hpp"
ENUM = SOURCE / "AudioKitSynthOne/DSP/S1Parameter.h"


def require(value, message):
    if not value:
        raise ValueError(message)


def emit(path, text):
    if path.name in ("parameter_map.hpp", "parameter_map.cpp"):
        require(path.read_text() == text, "immutable map changed: " + path.name)
    else:
        OUTPUT.mkdir(parents=True, exist_ok=True)
        (OUTPUT / path.name).write_text(text)


require(hashlib.sha256(HEADER.read_bytes()).hexdigest() == "95dd9af314495a56e5d2b9619c5737107b2a6d9ac3b9120ff657034f5b0dac32", "master header changed")
require(hashlib.sha256(ENUM.read_bytes()).hexdigest() == "9598182b88dd9fdc7f31332857647a63913e232cb40d85169978b2ccdf271783", "master enum changed")
source = HEADER.read_text()
metadata = source.split("S1ParameterInfo s1p[S1Parameter::S1ParameterCount] = {", 1)[1].split("\n    };", 1)[0]
pattern = r'\{\s*(\w+)\s*,\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*"[^"]*"\s*,\s*"[^"]*"\s*,\s*\w+\s*,\s*(true|false)\s*,\s*NULL\s*\}'
entries = re.findall(pattern, metadata)
require(len(entries) == 150, "expected exact master 150-entry metadata")
flags = [int(row[4] == "true") for row in entries]
cache_flags = [int(x) for x in re.search(r'smooth_flags\[PARAM_COUNT\]=\{([^}]+)\}', (ROOT / "../smoothing-cache-oracle/parameter_flags.h").read_text()).group(1).split(",")]
require(flags == cache_flags and sum(flags) == 46, "master/cache flag mismatch")
indices = [i for i, selected in enumerate(flags) if selected]
slots = [indices.index(i) if flags[i] else 255 for i in range(150)]
out = ROOT / "generated"
out.mkdir(exist_ok=True)
emit(out / "parameter_map.hpp", '''// Generated from pinned public master metadata. MIT notices in the parent experiment.
#pragma once
#include <cstddef>
#include <cstdint>
namespace synthone_ports {
constexpr unsigned parameter_count = 150;
constexpr unsigned selected_count = 46;
constexpr unsigned half_time_parameter = 144;
constexpr uint8_t unsmoothed_slot = 255;
extern const uint8_t selected_indices[selected_count];
extern const uint8_t selected_slots[parameter_count];
}
''')
emit(out / "parameter_map.cpp", '#include "parameter_map.hpp"\nnamespace synthone_ports {\nconst uint8_t selected_indices[selected_count] = {' + ','.join(map(str, indices)) + '};\nconst uint8_t selected_slots[parameter_count] = {' + ','.join(map(str, slots)) + '};\n}\n')
enum_source = ENUM.read_text().split("typedef AK_ENUM(S1Parameter) {", 1)[1].split("} S1Parameter;", 1)[0]
constants = source.split("    const float minutesPerSecond", 1)[1].split("    S1ParameterInfo s1p[", 1)[0]
reference = '''// Source expressions preserved from master; no owner preset values.
#pragma once
namespace source_metadata {
enum S1Parameter {''' + enum_source + '''};
constexpr int S1_NUM_BANDLIMITED_FTABLES = 13;
const float minutesPerSecond''' + constants + '''
struct Parameter { S1Parameter declared_parameter; float minimum, initial, maximum; bool selected; };
static const Parameter parameters[150] = {
'''
reference += ''.join('    {' + ','.join(row[:4]) + ',' + row[4] + '},\n' for row in entries)
reference += '};\n}\n'
emit(out / "reference_metadata.hpp", reference)
emit(out / "metadata.json", json.dumps({"master_commit": "6466a3715c96b7ecf1dd255a218cbd571408a314", "parameters": 150, "selected": indices, "slot_sentinel": 255, "metadata_index_136_declared_parameter": entries[136][0]}, indent=2) + "\n")
print(json.dumps({"parameters": 150, "selected": len(indices), "immutable_map_bytes": len(indices) + len(slots)}))
