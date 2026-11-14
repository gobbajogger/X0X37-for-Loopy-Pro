#!/usr/bin/env python3
"""Expected-failure sensitivity checks. Compile failures never count as kills."""
import json,os,shlex,subprocess,tempfile
from pathlib import Path
h=Path(__file__).resolve().parent
src=Path(os.environ['SOUNDPIPE_ROOT']).resolve();cc=shlex.split(os.environ.get('CC','cc'))
test=(h/'test_cache_list.c').read_text();candidate=(h/'cache_candidate.h').read_text()
cases=[
 ('omit_selected', 'candidate', 'return n;', 'return n-1;', 'FAIL selected parameter count'),
 ('duplicate_selected','candidate','return n;', 'out[n-1]=out[0]; return n;', 'FAIL duplicated parameter'),
 ('stale_coefficient','candidate','p->c2=pow(0.5,p->onedsr/p->htime);','p->c2=0.5;', 'FAIL output'),
 ('update_unsmoothed','candidate','if(smooth_flags[i])out[n++]=i;','out[n++]=i;', 'FAIL selected parameter count'),
 ('omit_half_time_refresh','test','ref[i].htime=opt[i].htime=h;prepare_cache(&opt[i]);','ref[i].htime=opt[i].htime=h;if(frame==0)prepare_cache(&opt[i]);', 'FAIL output sr=44100 frame=64'),
 ('omit_reset','test','sp_port_reset(&sp,&opt[i],&seed);','if(frame==0)sp_port_reset(&sp,&opt[i],&seed);', 'FAIL output sr=44100 frame=257'),
 ('corrupt_unsmoothed','test','for(unsigned i=0;i<PARAM_COUNT;i++){\n    if(bits(a[i])','b[3]+=1.0f;for(unsigned i=0;i<PARAM_COUNT;i++){\n    if(bits(a[i])', 'FAIL output sr=44100 frame=0 parameter=3'),
 ('omit_parameter_output','test','b[i]=cached_tick(&opt[i],target[i]);','(void)cached_tick(&opt[i],target[i]);', 'FAIL output'),
]
results=[]
for name,which,old,new,expected in cases:
    baseline=candidate if which=='candidate' else test
    assert baseline.count(old)==1,(name,baseline.count(old))
    with tempfile.TemporaryDirectory(prefix='synthone-cache-mutation-') as d:
        p=Path(d);(p/'parameter_flags.h').write_bytes((h/'parameter_flags.h').read_bytes())
        (p/'cache_candidate.h').write_text(candidate.replace(old,new) if which=='candidate' else candidate)
        (p/'test_cache_list.c').write_text(test.replace(old,new) if which=='test' else test)
        compile_run=subprocess.run(cc+['-std=c99','-O2','-ffp-contract=off','-fno-fast-math','-I'+str(src),'-I'+str(src/'lib/dr_wav'),str(p/'test_cache_list.c'),str(src/'modules/port.c'),'-lm','-o',str(p/'mutant')],capture_output=True,text=True)
        if compile_run.returncode: raise RuntimeError(name+' compile failed: '+compile_run.stderr)
        r=subprocess.run([str(p/'mutant')],capture_output=True,text=True)
        out=r.stdout+r.stderr
        assert r.returncode==1 and expected in out,(name,r.returncode,out)
        results.append({'name':name,'compile_exit':compile_run.returncode,'test_exit':r.returncode,'diagnostic':out.strip()})
        print('MUTANT_KILLED '+name+' '+out.strip())
(h/'mutation-results.json').write_text(json.dumps({'expected_failures':len(results),'results':results},indent=2)+'\n')
print('SENSITIVITY_PASS expected_failures='+str(len(results)))
