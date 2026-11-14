#!/usr/bin/env python3
"""Verify external public source pins and this test's metadata/recurrence contract."""
import hashlib,json,os,re
from pathlib import Path
HERE=Path(__file__).resolve().parent
roots={k:Path(os.environ[k.upper()+'_ROOT']).resolve() for k in ('master','develop','audiokit')}
assert Path(os.environ['SOUNDPIPE_ROOT']).resolve()==(roots['audiokit']/'AudioKit/Core/Soundpipe').resolve(), 'SOUNDPIPE_ROOT must resolve to the pinned AUDIOKIT_ROOT subtree'
m=json.loads((HERE/'input-manifest.json').read_text())
for s in m['sources']:
    p=roots[s['root']]/s['path']
    assert hashlib.sha256(p.read_bytes()).hexdigest()==s['sha256'], str(p)
def rows(root):
    h=(roots[root]/'AudioKitSynthOne/DSP/Kernel/S1DSPKernel.hpp').read_text()
    h=h.split('S1ParameterInfo s1p[S1Parameter::S1ParameterCount] = {',1)[1].split('\n    };',1)[0]
    r=re.findall(r'\{\s*(\w+),[^\n]*,\s*(true|false),\s*NULL\s*\}',h)
    assert len(r)==150, (root,len(r))
    return r
master,develop=rows('master'),rows('develop')
a=[int(v=='true') for _,v in master];b=[int(v=='true') for _,v in develop]
assert a==b and sum(a)==46
flags=[int(v) for v in re.search(r'smooth_flags\[PARAM_COUNT\]=\{([^}]+)\}',(HERE/'parameter_flags.h').read_text())[1].split(',')]
assert a==flags
assert [(i,a[0],b[0],a[1]) for i,(a,b) in enumerate(zip(master,develop)) if a!=b]==[(136,'compressorMasterMakeupGain','compressorReverbWetMakeupGain','false')]
def compact(s): return re.sub(r'\s+','',s)
parameters=compact((roots['develop']/'AudioKitSynthOne/DSP/Kernel/S1DSPKernel+parameters.mm').read_text())
process=compact((roots['develop']/'AudioKitSynthOne/DSP/Kernel/S1DSPKernel+process.mm').read_text())
init=compact((roots['develop']/'AudioKitSynthOne/DSP/Kernel/S1DSPKernel.mm').read_text())
assert 'port->c2=pow(0.5,port->onedsr/port->htime);port->c1=1.0-port->c2;port->prvhtim=port->htime;' in parameters
assert 'parameters[parameterIndex]=p->yt1=p->c1*parameterInfo->portamentoTarget+p->c2*p->yt1;' in process
assert 'if(s1p[i].usePortamento){portamentoParameterIndexes.push_back(i);}' in init
candidate=compact((HERE/'cache_candidate.h').read_text())
assert 'if(smooth_flags[i])out[n++]=i;' in candidate
assert 'p->c2=pow(0.5,p->onedsr/p->htime);p->c1=1.0-p->c2;p->prvhtim=p->htime;' in candidate
assert 'returnp->yt1=p->c1*target+p->c2*p->yt1;' in candidate
print('SOURCE_CONTRACT_PASS pinned_files=7 parameter_flags=150 smoothed=46 name_difference_index=136')
