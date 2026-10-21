#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Schema3 scoped DEV evidence for the explicit, segmented SMC test profile.
No release gate or hardware action. Generic schema2 records cannot select it.
"""
import json
import os
from pathlib import Path
import platform
import shutil
import signal
import subprocess
import sys
import tempfile

PROFILE='smc-vendor-central-dev'

def verify(repo,commit,record,fwsc):
    import dev_validate as dv
    import smc_vendor_bundle as bundle
    path=Path(record).resolve();data=json.loads(path.read_text())
    if (data.get('schema')!=3 or data.get('result')!='DEV CHECKED' or data.get('profile')!=PROFILE or
        data.get('config')!='dev' or data.get('model')!='elite' or data.get('hardware_tested') is not False):
        raise ValueError('not an Elite SMC central DEV record')
    if data.get('tree')!=dv.tree(repo,commit):raise ValueError('SMC evidence source tree mismatch')
    dv.validation_inputs.validate(data.get('validation_inputs'))
    inputs=data['validation_inputs']['inputs']
    config={'model':'elite','config':'dev','profile':PROFILE,'controller':data['controller'],
            'plan':data['plan'],'dependencies':data['plan'].get('dependencies',[]),
            'layout':'abi4-primary84000-uiAB000-roAD000-loadFE000'}
    if inputs.get('source')!=data['tree'] or inputs.get('mode')!='dev' or inputs.get('configuration')!=config:
        raise ValueError('SMC validation input profile/layout mismatch')
    dv.check_plan(data['plan'])
    expected={'build'}|{f'check-{i}' for i in range(len(data['plan'].get('checks',[])))}
    if set(data['logs'])!=expected:raise ValueError('missing SMC build/check logs')
    for name,info in data['logs'].items():
        if info['returncode']!=0 or dv.digest(path.parent/(name+'.log'))!=info['sha256']:
            raise ValueError('SMC validation log failed or changed: '+name)
    if dv.budgets((path.parent/'build.log').read_text())!=data['budgets']:
        raise ValueError('SMC build budget evidence mismatch')
    if dv.digest(fwsc)!=data['artifacts']['package']['sha256']:
        raise ValueError('SMC package differs from checked artifact')
    actual=bundle.validate_bundle(Path(fwsc).read_bytes(),loader_binding=data['loader_binding'])
    if actual!=data['smc_vendor'] or actual['controller']['kind']!='segmented-abi4' or data['controller']!={'kind':'segmented-abi4','retained_elf_sha256':__import__('smc_vendor_dev_build').ELF_SHA}:
        raise ValueError('SMC paired component identity mismatch')
    return data

def run(a):
    import dev_validate as dv
    import smc_vendor_dev_build as builder
    import smc_vendor_bundle as bundle
    import fm1pkg_make as pm
    if a.model!='elite' or a.no_controller or a.worker_image or not a.smc_vendor_dir:
        raise ValueError('SMC DEV needs Elite, explicit vendor directory and native Linux; no --no-controller/worker')
    if platform.system()!='Linux' or platform.machine() not in ('x86_64','AMD64'):
        raise ValueError('SMC DEV target build requires Linux x86-64')
    repo=Path(a.repo).resolve();commit=dv.git(repo,'rev-parse','--verify',a.commit+'^{commit}')
    plan=json.loads(Path(a.plan).read_text());checks=dv.check_plan(plan)
    if 'worker' in plan:raise ValueError('worker provenance is not accepted for this native profile')
    base=dv.git(repo,'rev-parse','--verify',plan['base']+'^{commit}')
    subprocess.run(['git','-C',str(repo),'merge-base','--is-ancestor',base,commit],check=True)
    plan['base']=base
    vendor=Path(a.smc_vendor_dir).resolve()
    dependencies=list(plan.get('dependencies',[]))
    for name in ('result.json','resolved-NOT-FIRMWARE.elf'):
        p=str(vendor/name)
        if p not in dependencies:dependencies.append(p)
    plan['dependencies']=dependencies
    out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=False)
    env=dv.development_env(os.environ,'elite')
    env.setdefault('JIELI_TOOLCHAIN',str(Path.home()/'.jieli/toolchain'))
    env.setdefault('AC79_SDK',str(Path.home()/'fw-AC79_AIoT_SDK'))
    tmp=Path(tempfile.mkdtemp(prefix='smc-dev-check-',dir=repo.parent));work=tmp/'source'
    added=False;logs={}
    try:
        subprocess.run(['git','-C',str(repo),'worktree','add','--detach',str(work),commit],check=True);added=True
        builder.retained_vendor(work,vendor)
        # Controller identity is completed from the exact bundle below. Capture
        # immutable inputs before any build using the deterministic sealed ELF.
        controller={'kind':'segmented-abi4','retained_elf_sha256':builder.ELF_SHA}
        config={'model':'elite','config':'dev','profile':PROFILE,'controller':controller,'plan':plan,
                'dependencies':dependencies,'layout':'abi4-primary84000-uiAB000-roAD000-loadFE000'}
        def identity():
            return dv.validation_inputs.capture(repo,commit,'dev',config,env['JIELI_TOOLCHAIN'],env['AC79_SDK'],env=env)
        inputs=identity()
        def command(name,args):
            with (out/(name+'.log')).open('w') as f:
                process=subprocess.Popen(args,cwd=work,env=env,stdout=f,stderr=subprocess.STDOUT,start_new_session=True)
                try:rc=process.wait(timeout=a.timeout)
                except BaseException:
                    try:os.killpg(process.pid,signal.SIGKILL)
                    except ProcessLookupError:pass
                    process.wait();raise
            logs[name]={'argv':args,'returncode':rc,'sha256':dv.digest(out/(name+'.log'))}
            if rc:raise ValueError(name+' failed; see '+str(out/(name+'.log')))
        for i,check in enumerate(checks):command(f'check-{i}',check['argv'])
        command('build',[sys.executable,'tools/smc_vendor_dev_build.py','--repo',str(work),
            '--out',str(out/'component-build'),'--vendor-dir',str(vendor),
            '--profile',PROFILE,'--model','elite','--config','dev'])
        root=out/'component-build';c=root/'components';meta=json.loads((root/'components.json').read_text())
        # No SDK or toolchain is distributed. Only ordinary generated firmware
        # bytes enter this explicitly named constructor; protected head is never written.
        pm.set_board('smk37');pm.SDK=env['AC79_SDK']
        app=(c/'app.bin').read_bytes()
        raw=bundle.build_bundle(pm.flash_image(app,pm.KEY),app,(c/'ui.bin').read_bytes(),
            [(c/f'vendor-{i}.bin').read_bytes() for i in range(3)],(c/'ota.bin').read_bytes(),
            descriptor_off=meta['loader_binding']['descriptor_off'],loader_binding=meta['loader_binding'])
        package=out/'x0x-smk37-elite-smc-central-dev.fwsc';package.write_bytes(raw)
        bundlemeta=bundle.validate_bundle(raw,loader_binding=meta['loader_binding'])
        # Real budget lines, not the component wrapper's prose, are authoritative.
        buildlog=(root/'build.log').read_text()
        with (out/'build.log').open('a') as f:f.write('\nFINAL TARGET BUDGETS\n'+buildlog)
        logs['build']['sha256']=dv.digest(out/'build.log')
        limits=dv.budgets(buildlog)
        if limits['code_free']!=meta['app_free']:raise ValueError('fixed app budget differs')
        if dv.git(work,'status','--porcelain') or dv.git(work,'rev-parse','HEAD')!=commit:
            raise ValueError('checks changed sources')
        dv.validation_inputs.validate(inputs,identity())
        data={'schema':3,'result':'DEV CHECKED','profile':PROFILE,'model':'elite','config':'dev',
            'hardware_tested':False,'commit':commit,'tree':dv.tree(repo,commit),'plan':plan,
            'validation_inputs':inputs,'controller':controller,'smc_vendor':bundlemeta,
            'loader_binding':meta['loader_binding'],'budgets':limits,'logs':logs,
            'artifacts':{'package':{'file':package.name,'sha256':dv.digest(package)}},
            'changed_files':dv.git(repo,'diff','--name-only',base,commit).splitlines()}
        # Keep the validation input controller identity stable, but separately
        # bind complete segment identities in bundle metadata.
        data['controller']=controller
        (out/'record.json').write_text(json.dumps(data,indent=2)+'\n')
    finally:
        if added:subprocess.run(['git','-C',str(repo),'worktree','remove','--force',str(work)],check=True)
        shutil.rmtree(tmp)
    print('DEV CHECKED '+str(out/'record.json'))
