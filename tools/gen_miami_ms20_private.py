#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Private N513 only. Cutoff-cap array is deliberately excluded pending correction."""
import argparse,hashlib,json,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('manifest');p.add_argument('output');a=p.parse_args()
b=Path(a.manifest).read_bytes();assert hashlib.sha256(b).hexdigest()=='26bffc368d7bedba4bb5cb8df17dda9e2e18e13c2216a4affc5f5987704eef98'
x=json.loads(b)['nonlinearity_N513'];values=x['uint32_bits'];assert len(values)==513
assert hashlib.sha256(struct.pack('<513I',*values)).hexdigest()=='4880416fdae25c3fef0c3885c358f65a5f42d4470f0f6e7d8d8922a115de800c'
s='/* PRIVATE functional coefficients; no public delivery authority. */\n#include <stdint.h>\nstatic const uint32_t original_miami_n513_bits[513]={\n'
s+='\n'.join(','.join(f'0x{v:08x}u' for v in values[i:i+8])+',' for i in range(0,len(values),8))+'\n};\n'
Path(a.output).write_text(s)
