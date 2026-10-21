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


def checked_tools(repo,commit):
    """Constructor/validator implementation must be the selected source, too."""
    here=Path(__file__).resolve().parent
    for name in ('smc_vendor_dev_validate.py','smc_vendor_dev_build.py','smc_vendor_bundle.py',
                 'dev_validate.py','validation_inputs.py','fm1pkg_make.py','add_ctl.py','smc_vendor_replay.py','smc_vendor_install.py','fm1_install.py','require_gate_pass.sh','smc_head_export_record.py','smc_head_export_replay.py',
                 'studies/smc_head_export/acquire.py','studies/smc_head_export/host.py',
                 'studies/smc_head_export/prepare.py','studies/smc_head_export/head_export.h',
                 'studies/smc_head_export/loader_core.h','studies/smc_head_export/build_source.py'):
        expected=subprocess.check_output(['git','-C',str(repo),'show',str(commit)+':tools/'+name])
        if (here/name).read_bytes()!=expected:
            raise ValueError('invoking tool differs from selected commit: '+name)

def verify(repo,commit,record,fwsc):
    import dev_validate as dv
    import smc_vendor_bundle as bundle
    import smc_vendor_dev_build as builder
    path=Path(record).resolve();data=json.loads(path.read_text())
    if (data.get('schema')!=3 or data.get('result')!='DEV CHECKED' or data.get('profile')!=PROFILE or
        data.get('config')!='dev' or data.get('model')!='elite' or data.get('hardware_tested') is not False):
        raise ValueError('not an Elite SMC central DEV record')
    checked_tools(repo,commit)
    if data.get('tree')!=dv.tree(repo,commit):raise ValueError('SMC evidence source tree mismatch')
    dv.validation_inputs.validate(data.get('validation_inputs'))
    inputs=data['validation_inputs']['inputs']
    config={'model':'elite','config':'dev','profile':PROFILE,'controller':data['controller'],
            'plan':data['plan'],'dependencies':data['plan'].get('dependencies',[]),
            'layout':'abi4-primary84000-uiAB000-roAD000-loadFE000'}
    if inputs.get('source')!=data['tree'] or inputs.get('mode')!='dev' or inputs.get('configuration')!=config:
        raise ValueError('SMC validation input profile/layout mismatch')
    dv.check_plan(data['plan'])
    expected={'build','bundle-replay','rollback-replay','head-replay'}|{f'check-{i}' for i in range(len(data['plan'].get('checks',[])))}
    if set(data['logs'])!=expected:raise ValueError('missing SMC build/check logs')
    for name,info in data['logs'].items():
        if info['returncode']!=0 or dv.digest(path.parent/(name+'.log'))!=info['sha256']:
            raise ValueError('SMC validation log failed or changed: '+name)
    if dv.budgets((path.parent/'build.log').read_text())!=data['budgets']:
        raise ValueError('SMC build budget evidence mismatch')
    if dv.digest(fwsc)!=data['artifacts']['package']['sha256']:
        raise ValueError('SMC package differs from checked artifact')
    import smc_head_export_record as head_record
    hi=data['artifacts']['head_export']
    hp=path.parent/hi['file']
    if dv.digest(hp)!=hi['sha256'] or head_record.validate(repo,hp)!=data['head_export']:
        raise ValueError('reviewed exporter identity mismatch')
    actual=bundle.validate_bundle(Path(fwsc).read_bytes(),loader_binding=data['loader_binding'])
    policy=actual['policy']
    if policy == 1:
        if tuple(r['sha256'] for r in actual['controller']['regions'])!=builder.REGION_SHA:
            raise ValueError('controller regions differ from reviewed pinned ELF')
        role='forward'
    elif policy == 2:
        # validate_bundle authorizes ONLY the exact approved 816 canonical pair.
        role='logical-rollback'
    else:raise ValueError('unsupported paired package policy')
    if (actual!=data['smc_vendor'] or data.get('artifact_role')!=role or
        data.get('budget_scope')!='built-central-test-profile' or
        data['controller']!={'kind':'segmented-abi4','retained_elf_sha256':builder.ELF_SHA}):
        raise ValueError('SMC paired component identity mismatch')
    return data

def run(a):
    import dev_validate as dv
    import smc_vendor_dev_build as builder
    import smc_vendor_bundle as bundle
    import fm1pkg_make as pm
    if a.model!='elite' or a.no_controller or a.worker_image or not a.smc_vendor_dir or not a.smc_rollback_fwsc or not a.smc_head_export_source:
        raise ValueError('SMC DEV needs Elite, explicit vendor directory and native Linux; no --no-controller/worker')
    if platform.system()!='Linux' or platform.machine() not in ('x86_64','AMD64'):
        raise ValueError('SMC DEV target build requires Linux x86-64')
    repo=Path(a.repo).resolve();commit=dv.git(repo,'rev-parse','--verify',a.commit+'^{commit}')
    checked_tools(repo,commit)
    plan=json.loads(Path(a.plan).read_text());checks=dv.check_plan(plan)
    if 'worker' in plan:raise ValueError('worker provenance is not accepted for this native profile')
    base=dv.git(repo,'rev-parse','--verify',plan['base']+'^{commit}')
    subprocess.run(['git','-C',str(repo),'merge-base','--is-ancestor',base,commit],check=True)
    plan['base']=base
    vendor=Path(a.smc_vendor_dir).resolve()
    baseline=Path(a.smc_rollback_fwsc).resolve()
    head_source=Path(a.smc_head_export_source).resolve()
    if dv.digest(baseline)!=bundle.BASELINE_FWSC_SHA:raise ValueError('exact approved rollback package required')
    dependencies=list(plan.get('dependencies',[]))
    for p in (baseline,head_source):
        if str(p) not in dependencies:dependencies.append(str(p))
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
        import smc_head_export_record as head_record
        headmeta=head_record.validate(work,head_source)
        headcopy=out/'elite-head-export-dev.fwsc';shutil.copyfile(head_source,headcopy)
        for i,check in enumerate(checks):command(f'check-{i}',check['argv'])
        command('head-replay',[sys.executable,'tools/smc_head_export_replay.py',str(headcopy),str(out/'head-replay-private')])
        command('build',[sys.executable,'tools/smc_vendor_dev_build.py','--repo',str(work),
            '--out',str(out/'component-build'),'--vendor-dir',str(vendor),
            '--profile',PROFILE,'--model','elite','--config','dev'])
        root=out/'component-build';c=root/'components';meta=json.loads((root/'components.json').read_text())
        for name,info in meta['artifacts'].items():
            if (c/name).stat().st_size!=info['size'] or dv.digest(c/name)!=info['sha256']:
                raise ValueError('component changed after checked build: '+name)
        # No SDK or toolchain is distributed. Only ordinary generated firmware
        # bytes enter this explicitly named constructor; protected head is never written.
        pm.set_board('smk37');pm.SDK=env['AC79_SDK']
        app=(c/'app.bin').read_bytes()
        raw=bundle.build_bundle(pm.flash_image(app,pm.KEY),app,(c/'ui.bin').read_bytes(),
            [(c/f'vendor-{i}.bin').read_bytes() for i in range(3)],(c/'ota.bin').read_bytes(),
            descriptor_off=meta['loader_binding']['descriptor_off'],loader_binding=meta['loader_binding'])
        package=out/'x0x-smk37-elite-smc-central-dev.fwsc';package.write_bytes(raw)
        bundlemeta=bundle.validate_bundle(raw,loader_binding=meta['loader_binding'])
        if tuple(r['sha256'] for r in bundlemeta['controller']['regions'])!=builder.REGION_SHA:
            raise ValueError('constructed controller is not the reviewed pinned image')
        rollback=out/'x0x-smk37-elite-logical-rollback-816.fwsc'
        rollback.write_bytes(bundle.build_rollback(baseline.read_bytes(),(c/'ota.bin').read_bytes(),loader_binding=meta['loader_binding']))
        rollbackmeta=bundle.validate_bundle(rollback.read_bytes(),loader_binding=meta['loader_binding'])
        command('rollback-replay',[sys.executable,'tools/smc_vendor_replay.py',str(rollback),str(root)])
        command('bundle-replay',[sys.executable,'tools/smc_vendor_replay.py',str(package),str(root)])
        # Real budget lines, not the component wrapper's prose, are authoritative.
        buildlog=(root/'build.log').read_text()
        with (out/'build.log').open('a') as f:f.write('\nFINAL TARGET BUDGETS\n'+buildlog)
        logs['build']['sha256']=dv.digest(out/'build.log')
        limits=dv.budgets(buildlog)
        if limits['code_free']!=meta['app_free']:raise ValueError('fixed app budget differs')
        if dv.git(work,'status','--porcelain') or dv.git(work,'rev-parse','HEAD')!=commit:
            raise ValueError('checks changed sources')
        dv.validation_inputs.validate(inputs,identity())
        data={'artifact_role':'forward','budget_scope':'built-central-test-profile','schema':3,'result':'DEV CHECKED','profile':PROFILE,'model':'elite','config':'dev',
            'hardware_tested':False,'head_export':headmeta,'commit':commit,'tree':dv.tree(repo,commit),'plan':plan,
            'validation_inputs':inputs,'controller':controller,'smc_vendor':bundlemeta,
            'loader_binding':meta['loader_binding'],'budgets':limits,'logs':logs,
            'artifacts':{'package':{'file':package.name,'sha256':dv.digest(package)},
                         'head_export':{'file':headcopy.name,'sha256':dv.digest(headcopy)}},
            'changed_files':dv.git(repo,'diff','--name-only',base,commit).splitlines()}
        # Keep the validation input controller identity stable, but separately
        # bind complete segment identities in bundle metadata.
        data['controller']=controller
        (out/'record.json').write_text(json.dumps(data,indent=2)+'\n')
        rollbackdata=dict(data,artifact_role='logical-rollback',smc_vendor=rollbackmeta,
            artifacts={'package':{'file':rollback.name,'sha256':dv.digest(rollback)},'head_export':data['artifacts']['head_export']})
        (out/'rollback-record.json').write_text(json.dumps(rollbackdata,indent=2)+'\n')
    finally:
        if added:subprocess.run(['git','-C',str(repo),'worktree','remove','--force',str(work)],check=True)
        shutil.rmtree(tmp)
    print('DEV CHECKED '+str(out/'record.json'))
