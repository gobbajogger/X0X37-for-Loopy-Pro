"""Time the existing macOS build path and compare the saved Linux package."""
from pathlib import Path
import os,subprocess,time,json,hashlib
root=Path(__file__).resolve().parents[2];out=Path(__file__).parent
env=os.environ.copy()
for key in list(env):
    if key.startswith('X0X_') or key in ('MODEL','BOARD','JIELI_DOCKER','FELUCCA_DATA_FAMILY','FELUCCA_DATA_UNTAGGED_OK'):
        env.pop(key,None)
env.update(MODEL='elite',BOARD='smk37',X0X_DEV_PADCAL='1',JIELI_TOOLCHAIN='/Users/mac/.jieli/toolchain',AC79_SDK='/Users/mac/fw-AC79_AIoT_SDK',CC='cc',CXX='c++',PYTHONDONTWRITEBYTECODE='1')
start=time.perf_counter()
with (out/'native-build.log').open('w') as log:
    result=subprocess.run(['sh','build.sh','--board','smk37'],cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT)
data={'returncode':result.returncode,'seconds':time.perf_counter()-start,'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),'config':'dev','model':'elite'}
if result.returncode==0:
    built=root/'build/x0x-smk37-elite.fwsc'
    reference=Path('/Users/mac/Documents/x0x37/smc_central_stop_stage4_20261021/evidence/stop-stage4/elite/x0x-smk37-elite.fwsc')
    for name,path in [('native_package',built),('linux_package',reference)]:
        data[name]={'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
    data['package_byte_equal']=built.read_bytes()==reference.read_bytes()
(out/'native-build-result.json').write_text(json.dumps(data,indent=2)+'\n')
print(json.dumps(data),flush=True)
raise SystemExit(result.returncode)
