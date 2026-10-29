#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Private original functional coefficients; not a publication grant."""
import sys,json,hashlib,struct
from pathlib import Path
b=Path(sys.argv[1]).read_bytes();assert hashlib.sha256(b).hexdigest()=='bb0fcb9314a99135ea0c662af5dedd476021e9747a2489d8ce821b407393e9f6'
d=json.loads(b);out='/* PRIVATE original functional coefficient data. */\n#include <stdint.h>\n'
for name,symbol in [('Drive','original_drive_table'),('Gain','original_drive_gain_table')]:
 t=d['functional_tables'][name];v=t['values_int16'];assert len(v)==128;assert hashlib.sha256(struct.pack('<128h',*v)).hexdigest()==t['sha256_le_int16']
 out+=f'static const int16_t {symbol}[128]={{'+','.join(map(str,v))+'};\n'
Path(sys.argv[2]).write_text(out)
print('PASS immutable Drive manifest and both 128-entry coefficient hashes')
