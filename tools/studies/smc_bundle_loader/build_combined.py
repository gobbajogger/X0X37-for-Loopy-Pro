#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build NON-INSTALLABLE app + separately staged paired loader evidence.
Requires pinned SDK/toolchain environment. Never calls package generation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import tempfile

BASE = '816e0d3987bcb69be972cff2cd4df4469969f196'
LOADER_BASE = 'ca6c21be5de21b990674dcf315be41262285dd10'

def run(*args, **kwargs):
    subprocess.run([str(x) for x in args], check=True, **kwargs)

def export(repo, revision, out):
    out.mkdir()
    with tempfile.TemporaryFile() as archive:
        run('git', '-C', repo, 'archive', revision, stdout=archive)
        archive.seek(0)
        with tarfile.open(fileobj=archive) as tf:
            tf.extractall(out, filter='data')

def build(out, log):
    text = (out/'tools/build.py').read_text()
    # The opt-in recipes must insert an unconditional return before packaging.
    package = text.index('    pkg = fm1pkg_make.ufw')
    if 'NON-INSTALLABLE' not in text[:package] or not text[:package].rstrip().endswith('return 0'):
        raise RuntimeError('packaging suppression is absent')
    env = dict(os.environ, BOARD='smk37', MODEL='elite', X0X_DEV_PADCAL='1')
    with log.open('w') as f:
        run('python3', 'tools/build.py', cwd=out, env=env, stdout=f, stderr=subprocess.STDOUT)
    if list((out/'build').glob('*.fwsc')):
        raise RuntimeError('unexpected package output')

def digest(p):
    b=p.read_bytes()
    return {'bytes':len(b), 'sha256':hashlib.sha256(b).hexdigest()}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    a=parser.parse_args(); repo=a.repo.resolve(); out=a.out.resolve()
    dirty=subprocess.check_output(['git','-C',str(repo),'status','--porcelain','--untracked-files=no'],text=True)
    if dirty:
        parser.error('tracked source inputs must be clean; commit the study inputs first')
    if out.exists() or out.is_relative_to(repo):
        parser.error('output must be new and outside source repository')
    out.mkdir(parents=True)
    app=out/'app'; loader=out/'loader-source'; studies=repo/'tools/studies'
    export(repo,BASE,app)
    for script in ('prepare.py','prepare_admission.py','prepare_bridge.py'):
        run('python3',studies/'smc_app_data'/script,app)
    build(app,out/'app-first.log')
    target=app/'build/smk37-elite'
    before=digest(target/'x0x.bin')
    if before['bytes']>524000: raise RuntimeError('fixed app partition exceeded')
    run('python3',studies/'smc_app_data/seal_fixture.py','--elf',target/'x0x.elf',
        '--app',target/'x0x.bin','--out',out/'sealed-app')
    export(repo,LOADER_BASE,loader)
    run('python3',studies/'smc_bundle_loader/prepare.py',loader,target/'x0x.elf',
        out/'sealed-app/app-NOT-INSTALLABLE.fixture')
    for name in ('bp_manifest.h','bp_pin.h','bundle_probe.h','record_owner.h',
                 'rollback_pins.h','crt0_ldr.S','ldr_core.c','loader.c'):
        shutil.copyfile(loader/'firmware/loader'/name,app/'firmware/loader'/name)
    shutil.copyfile(loader/'firmware/hal/bp_loader_quiesce.h',app/'firmware/hal/bp_loader_quiesce.h')
    build(app,out/'app-paired-loader.log')
    if digest(target/'x0x.bin') != before:
        raise RuntimeError('separate loader unexpectedly changed pinned app')
    result={'installable':False,'app_partition':524000,'app_margin':524000-before['bytes'],
            'baseline':BASE,'loader_source_baseline':LOADER_BASE,
            'source_commit':subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip(),
            'artifacts':{name:digest(target/name) for name in
                         ('x0x.bin','x0x.elf','x0x.map','loader/loader.bin','loader/ota.bin')}}
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if __name__=='__main__': main()
