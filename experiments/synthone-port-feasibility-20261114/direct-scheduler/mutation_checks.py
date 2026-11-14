#!/usr/bin/env python3
"""Compile bounded scheduler mistakes and require a runtime differential failure."""
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path
import subprocess
import sys

MUTATIONS=[
 ('per_frame_sparse_holes','if(note.rootNoteNumber!=-1 && note.stage!=S1NoteState::stageOff) note.run(offset+i,l,r);','note.run(offset+i,l,r);',1),
 ('release_threshold_inclusive','note.amp<.01f','note.amp<=.01f',1),
 ('missing_poly_extra_adsr','sp_adsr_compute(sp,note.adsr,&note.internalGate,&note.amp);','(void)note;',1),
 ('normalized_mono_reset','parameters[isMono]!=previousProcessMonoPolyStatus','(parameters[isMono]>0)!=(previousProcessMonoPolyStatus>0)',1),
 ('fallback_noise_cleanup','// The original fallback intentionally does not update noise->amp.','mono.noise->amp=mono.amp;',1),
 ('wrong_steal_cursor','selected<0?(playingNoteStatesIndex+1)%6:selected','selected<0?0:selected',1),
 ('reset_prng','mono.clear(); resetted=true;','mono.clear(); sp->rand=0; resetted=true;',1),
 ('late_parameter_tick','compactPorts.tick();\n        monoFrequencyPort->htime=parameters[glide];\n        sp_port_compute(sp,monoFrequencyPort,&monoFrequency,&monoFrequencySmooth);','monoFrequencyPort->htime=parameters[glide];\n        sp_port_compute(sp,monoFrequencyPort,&monoFrequency,&monoFrequencySmooth);\n        compactPorts.tick();',1),
 ('fresh_inverse_lfo_one','lfo1_1_0=lfo2_1_0=lfo3_1_0=0;','lfo1_1_0=lfo2_1_0=lfo3_1_0=1;',1),
 ('unclamped_oracle_output_half_time','p.portamento->htime=clampedValue(portamentoHalfTime,parameters[parameter]);','p.portamento->htime=parameters[parameter];',1),
]

def main():
    p=argparse.ArgumentParser();p.add_argument('--build',type=Path,required=True);p.add_argument('--tables',type=Path,required=True);a=p.parse_args()
    root=Path(__file__).resolve().parent;build=a.build.resolve()
    table_bytes=a.tables.resolve().read_bytes()
    expected=json.loads((root/'DEPENDENCIES.json').read_text())['table_payload']
    if len(table_bytes)!=expected['bytes'] or hashlib.sha256(table_bytes).hexdigest()!=expected['sha256']:
        raise SystemExit('negative-control table identity mismatch')
    commands=json.loads((build/'commands.json').read_text())
    compile_cmd=next(c for c in commands if any(x.endswith('/closure/scheduler/scheduler.cpp') for x in c))
    original_source=next(x for x in compile_cmd if x.endswith('/closure/scheduler/scheduler.cpp'))
    link_cmd=commands[-1]
    original_object=compile_cmd[compile_cmd.index('-o')+1]
    local=Path(original_source).parent
    source=Path(original_source).read_text();out=build/'mutations';out.mkdir(exist_ok=True)
    table=out/'public-morphs.f32';table.write_bytes(table_bytes)
    baseline=Path(link_cmd[link_cmd.index('-o')+1])
    before=subprocess.run([str(baseline),str(table)],text=True,capture_output=True)
    if before.returncode:raise SystemExit('negative-control baseline must pass before mutation: '+before.stderr)
    results=[]
    build_env=dict(os.environ)
    for variable in ['CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH','OBJC_INCLUDE_PATH']:build_env.pop(variable,None)
    for name,old,new,count in MUTATIONS:
        # The handwritten oracle setter is inline in a separate header. Rebuild
        # every local translation unit against an isolated copy, not stale objects.
        header_mutation=name=='unclamped_oracle_output_half_time'
        bound_source=(local/'oracle/state.hpp').read_text() if header_mutation else source
        if bound_source.count(old)!=count:raise SystemExit('mutation binding mismatch: '+name)
        obj=out/(name+'.o');binary=out/name
        if header_mutation:
            closure=out/(name+'-closure');shutil.copytree(local,closure,dirs_exist_ok=True)
            (closure/'oracle/state.hpp').write_text(bound_source.replace(old,new))
            replacements={}
            for original in commands[:-1]:
                if not any(x.startswith(str(local)+'/') and x.endswith('.cpp') for x in original):continue
                old_object=original[original.index('-o')+1]
                new_object=out/(name+'-'+Path(old_object).name);replacements[old_object]=str(new_object)
                c=[str(new_object) if x==old_object else x.replace(str(local),str(closure)) for x in original]
                compiled=subprocess.run(c,text=True,capture_output=True,env=build_env)
                (out/(name+'-'+Path(old_object).stem+'.compile.txt')).write_text(compiled.stdout+compiled.stderr)
                if compiled.returncode:raise SystemExit('mutation did not compile: '+name)
            l=[replacements.get(x,str(binary) if x==str(baseline) else x) for x in link_cmd]
        else:
            altered=out/(name+'.cpp');altered.write_text(bound_source.replace(old,new))
            c=[str(altered) if x==original_source else str(obj) if x==original_object else x for x in compile_cmd]
            l=[str(obj) if x==original_object else str(binary) if x==str(baseline) else x for x in link_cmd]
            compiled=subprocess.run(c,text=True,capture_output=True,env=build_env);(out/(name+'.compile.txt')).write_text(compiled.stdout+compiled.stderr)
            if compiled.returncode:raise SystemExit('mutation did not compile: '+name)
        linked=subprocess.run(l,text=True,capture_output=True,env=build_env)
        if linked.returncode:raise SystemExit('mutation did not link: '+name+' '+linked.stderr)
        executed=subprocess.run([str(binary),str(table)],text=True,capture_output=True)
        (out/(name+'.stderr.txt')).write_text(executed.stderr)
        detected=executed.returncode==1 and 'FAIL ' in executed.stderr
        results.append({'mutation':name,'compiled':True,'runtime_exit':executed.returncode,'detected':detected})
        if not detected:raise SystemExit('mutation escaped or failed setup: '+name+' '+executed.stderr)
    result={'scope':'Synthetic scheduler negative controls only','negative_controls':results,'all_detected':True}
    (out/'RESULT.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main()
