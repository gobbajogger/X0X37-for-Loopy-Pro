#!/usr/bin/env python3
"""Check retired-input cleanup without weakening active provenance or executing builds."""
import ast,hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];E=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
sys.path.insert(0,str(ROOT/'tools'))
import validation_inputs as identity
import dev_validate

def functions(text):
 return {n.name:ast.dump(n,include_attributes=False) for n in ast.parse(text).body if isinstance(n,(ast.FunctionDef,ast.ClassDef))}
old=functions(subprocess.check_output(['git','show','cadeae0:tools/validation_inputs.py'],cwd=ROOT,text=True));new=functions((ROOT/'tools/validation_inputs.py').read_text())
assert all(new[k]==v for k,v in old.items() if k!='runtime')
olddev=functions(subprocess.check_output(['git','show','cadeae0:tools/dev_validate.py'],cwd=ROOT,text=True));newdev=functions((ROOT/'tools/dev_validate.py').read_text())
assert all(newdev[k]==v for k,v in olddev.items() if k!='LinuxWorker')
legacy={'EMSDK':'missing-unused-sdk','EM_CONFIG':'missing-unused-config','LLVM_ROOT':'missing-unused-llvm','BINARYEN_ROOT':'missing-unused-binaryen','EMCC_CFLAGS':'unused-flags','EMCC_TEST':'unused-selector'}
env={'PATH':os.environ['PATH'],'CC':'cc','CXX':'c++','CFLAGS':'-DIDENTITY_FIXTURE=1','MODEL':'elite','X0X_DEV_PADCAL':'1'}
clean=identity.runtime({},env);retired=identity.runtime({},dict(env,**legacy));assert clean==retired
assert 'emcc' not in clean['tools']
assert clean['environment']['CFLAGS']==env['CFLAGS']
assert set(clean['tools']) >= {'python','cc','c++','bash','sh','git','node','ld','ar','objcopy'}
with tempfile.TemporaryDirectory(prefix='x0x-native-cleanup-') as temp:
 explicit=Path(temp)/'explicit-tool';explicit.write_text('#!/bin/sh\nexit 0\n');explicit.chmod(0o755)
 actual=identity.runtime({'plan':{'checks':[{'argv':[str(explicit)]}]}},env)
 assert actual['tools'][str(explicit)]==identity.content(explicit)
# Exercise actual start/forwarding logic with Docker protocol replies in memory.
worker=dev_validate.LinuxWorker.__new__(dev_validate.LinuxWorker)
worker.mounts={};worker.name='owned-fixture';worker.image={'Id':'sha256:fixture'};worker.created=False
worker.host_env=dict(env,**legacy,X0X_JOBS='3',X0X_HOST_DEFS='-DACTIVE=1',X0X_VALIDATION_PYTHON_PACKAGES='pillow')
worker.call=lambda *args,**kw:'PATH=/usr/bin:/bin\0HOME=/root\0' if args[0]=='exec' else ''
worker.start()
assert all(k not in worker.env for k in legacy)
assert worker.env['CFLAGS']==env['CFLAGS'] and worker.env['X0X_JOBS']=='3'
assert worker.env['X0X_HOST_DEFS']=='-DACTIVE=1' and worker.env['PATH']=='/usr/bin:/bin'
assert worker.env['CTL_NATIVE']=='1' and worker.env['CTL_NO_CACHE']=='1'
# All normalized dependency declarations still participate in identity.
from tests.host.test_validation_inputs import Inputs
fixture=Inputs('test_split_execution_preserves_all_original_input_bytes');fixture.setUp()
try:
 external=fixture.root/'external-input';external.write_text('first')
 fixture.config['dependencies']=[str(external)]
 before=fixture.capture();external.write_text('second');after=fixture.capture()
 try:identity.validate(before,after)
 except ValueError:pass
 else:raise AssertionError('Declared dependency mutation was accepted')
finally:fixture.doCleanups()
result={'result':'PASS','preserved_ast':'All native input helper functions except runtime, all dev_validate top-level functions except LinuxWorker exactly match acceptedcadeae0.','runtime':'Retired settings ignored; native compiler/tools and Python package byte identity unchanged; explicitly declared command still captured.','worker':'Only active forwarded inputs retained; Linux PATH/defaults and controller-native/no-cache development flags preserved.','dependencies':'Declared external input mutation invalidates captured identity.','limits':'Forwarding uses fake Docker protocol replies; separate real worker provenance check covers actual execution. No target/controller build.'}
(E/'native-cleanup.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
