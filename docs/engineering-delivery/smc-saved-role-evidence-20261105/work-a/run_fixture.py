#!/usr/bin/env python3
"""Run the pinned, unchanged fixture; capture raw process output and identity."""
import datetime
import hashlib
import json
import os
import pathlib
import platform
import shutil
import subprocess
import sys

BASE = pathlib.Path(__file__).resolve().parent
env = os.environ.copy()
env['CC'] = '/usr/bin/python3 ' + str(BASE / 'capture_cc.py')
cmd = ['python3', 'tests/host/test_smc_boot_saved_roles.py']
started = datetime.datetime.now(datetime.timezone.utc).isoformat()
proc = subprocess.run(cmd, cwd=BASE / 'closure', env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
(BASE / 'fixture.stdout').write_bytes(proc.stdout)
(BASE / 'fixture.stderr').write_bytes(proc.stderr)
record = {'started_utc': started, 'finished_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'argv': cmd, 'cwd': str(BASE / 'closure'), 'explicit_environment': {'CC': env['CC']}, 'exit': proc.returncode, 'stdout_sha256': hashlib.sha256(proc.stdout).hexdigest(), 'stderr_sha256': hashlib.sha256(proc.stderr).hexdigest(), 'python': sys.version, 'python_executable': sys.executable, 'python3_path': shutil.which('python3'), 'platform': platform.platform(), 'uname': list(platform.uname()), 'compiler': {}}
for label, cmdline in [('version', ['/usr/bin/cc', '--version']), ('target', ['/usr/bin/cc', '-dumpmachine']), ('fullversion', ['/usr/bin/cc', '-dumpfullversion'])]:
    info = subprocess.run(cmdline, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    record['compiler'][label] = {'argv': cmdline, 'exit': info.returncode, 'stdout': info.stdout.decode(), 'stderr': info.stderr.decode()}
(BASE / 'fixture-run.json').write_text(json.dumps(record, indent=2) + '\n')
sys.stdout.buffer.write(proc.stdout)
sys.stderr.buffer.write(proc.stderr)
print('FIXTURE_EXIT=' + str(proc.returncode), flush=True)
sys.exit(proc.returncode)
