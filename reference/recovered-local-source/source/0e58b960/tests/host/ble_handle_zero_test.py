#!/usr/bin/env python3
"""Compile production central C and replay valid handle zero through both peers.

--legacy-root runs the same discovery replay on the e75 single-peer source.
HCI/time are simulated; this is not RF/device timing evidence.
"""
import argparse,os,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[2])
p.add_argument('--uuid-include',type=Path)
p.add_argument('--legacy-root',type=Path)
a=p.parse_args();tests=Path(__file__).resolve().parent
cases=[('ble_dual_handle_test.c',base,False) for base in (0,1,64)]
cases += [('ble_discovery_replay_test.c',base,False) for base in (0,1,64)]
cases += [('ble_zero_handle_lifecycle_test.c',0,False)]
if a.legacy_root:cases += [('ble_discovery_replay_test.c',base,True) for base in (0,64)]
failures=0
with tempfile.TemporaryDirectory(prefix='ble-zero-handle-') as d:
 for name,base,legacy in cases:
  root=a.legacy_root if legacy else a.source_root
  exe=Path(d)/'test';cmd=[os.getenv('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-fno-sanitize-recover=all',f'-DTEST_HANDLE_BASE={base}',f'-DTEST_LEGACY={int(legacy)}','-I'+str(root/'firmware/src/ble')]
  if a.uuid_include:cmd+=['-I'+str(a.uuid_include)]
  cmd += [str(tests/name),'-o',str(exe)]
  b=subprocess.run(cmd,capture_output=True,text=True)
  r=subprocess.run([str(exe)],capture_output=True,text=True) if not b.returncode else b
  print(f'{name} base={base} legacy={int(legacy)} build={b.returncode} exit={r.returncode}: '+r.stdout.strip(),flush=True)
  if r.returncode:failures+=1;print(b.stderr+r.stderr,flush=True)
print(f'{len(cases)-failures}/{len(cases)} handle replay executions passed')
raise SystemExit(bool(failures))
