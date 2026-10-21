#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Replay exact reviewed export bytes through actual legacy OTA and export source."""
import os,subprocess,sys,tarfile
from pathlib import Path
import smc_head_export_record as record
import smc_vendor_bundle as b
if len(sys.argv)!=3:raise SystemExit('usage: smc_head_export_replay.py PACKAGE NEW_OUTPUT')
repo=Path(__file__).resolve().parents[1];package=Path(sys.argv[1]).resolve();out=Path(sys.argv[2]).resolve()
if out.exists() or out.is_relative_to(repo):raise ValueError('new output outside source required')
record.validate(repo,package);out.mkdir(parents=True);src=out/'source';src.mkdir()
archive=out/'source.tar'
with archive.open('wb') as f:subprocess.run(['git','-C',str(repo),'archive','816e0d3987bcb69be972cff2cd4df4469969f196'],stdout=f,check=True)
with tarfile.open(archive) as f:f.extractall(src,filter='data')
subprocess.run([sys.executable,str(repo/'tools/studies/smc_head_export/prepare.py'),str(src)],check=True)
logical=b.add_ctl.logical_of(package.read_bytes())[1];fixture=out/'logical.private';fixture.write_bytes(logical)
args=['cc','-std=c11','-Wall','-Wextra','-Wno-unused-function','-Werror',
 '-DHX_OTA_SOURCE="'+str(src/'firmware/src/ota.c')+'"',
 '-DHX_CORE_SOURCE="'+str(src/'firmware/loader/loader_core.h')+'"',
 str(repo/'tools/studies/smc_head_export/wire_test.c')]
for name,flags in [('native',['-O2']),('sanitized',['-O1','-g','-fsanitize=address,undefined'])]:
 exe=out/name;subprocess.run(args+flags+['-o',str(exe)],check=True)
 subprocess.run([str(exe),str(fixture)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
print('Reviewed exporter: actual legacy OTA session and head/cleanup wire replay native+ASan/UBSan PASS')
