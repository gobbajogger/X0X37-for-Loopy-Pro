#!/usr/bin/env python3
"""External transparent compiler capture; does not rewrite input or arguments."""
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys

BASE = pathlib.Path(__file__).resolve().parent
args = sys.argv[1:]
mode = 'ubsan' if '-fsanitize=undefined' in args else 'plain'
out = BASE / 'capture' / mode
out.mkdir(parents=True, exist_ok=True)
sources = [pathlib.Path(s) for s in args if s.endswith('.c')]
assert len(sources) == 1
source = sources[0]
shutil.copyfile(source, out / 'smc_boot.c')
real = '/usr/bin/cc'
cmd = [real, *args]
proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
(out / 'compile.stdout').write_bytes(proc.stdout)
(out / 'compile.stderr').write_bytes(proc.stderr)
sys.stdout.buffer.write(proc.stdout)
sys.stderr.buffer.write(proc.stderr)
record = {'capture_method': 'CC argv-transparent wrapper; copies source before calling real compiler; input bytes and compiler args unchanged', 'compiler_argv': cmd, 'compile_exit': proc.returncode, 'generated_c_size': source.stat().st_size, 'generated_c_sha256': hashlib.sha256(source.read_bytes()).hexdigest()}
if proc.returncode == 0:
    exe = pathlib.Path(args[args.index('-o') + 1])
    copied = out / exe.name
    shutil.copyfile(exe, copied)
    copied.chmod(0o700)
    record['binary_sha256'] = hashlib.sha256(copied.read_bytes()).hexdigest()
    for tool, flags in [('ldd', []), ('readelf', ['-d'])]:
        check = subprocess.run([tool, *flags, str(copied)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (out / (tool + '.stdout')).write_bytes(check.stdout)
        (out / (tool + '.stderr')).write_bytes(check.stderr)
        record[tool + '_exit'] = check.returncode
(out / 'compile.json').write_text(json.dumps(record, indent=2) + '\n')
sys.exit(proc.returncode)
