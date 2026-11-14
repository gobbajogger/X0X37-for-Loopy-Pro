#!/usr/bin/env python3
"""Build only this changed scheduler closure from explicit verified inputs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--experiment-root',type=Path,required=True)
    p.add_argument('--controls-root',type=Path,required=True)
    p.add_argument('--master-root',type=Path,required=True)
    p.add_argument('--tables',type=Path,required=True)
    p.add_argument('--variant',choices=['o2','os','sanitize'],default='o2')
    p.add_argument('--smoke',action='store_true',help='Run only the portable consumer binding smoke, preserving full prior fixture evidence separately')
    p.add_argument('--build-root',type=Path,help='Optional isolated directory for this run and its staged closure')
    a=p.parse_args()
    root=Path(__file__).resolve().parent
    bases={'experiment':a.experiment_root.resolve(),'controls':a.controls_root.resolve(),'master':a.master_root.resolve()}
    manifest=json.loads((root/'DEPENDENCIES.json').read_text())
    verified={}
    for group,entries in manifest['groups'].items():
        verified[group]={}
        for entry in entries:
            data=(bases[group]/entry['path']).read_bytes()
            if len(data)!=entry['bytes'] or hashlib.sha256(data).hexdigest()!=entry['sha256']:
                raise SystemExit('dependency identity mismatch: '+group+'/'+entry['path'])
            verified[group][entry['path']]=data
    tables=a.tables.resolve();data=tables.read_bytes();expected=manifest['table_payload']
    if len(data)!=expected['bytes'] or hashlib.sha256(data).hexdigest()!=expected['sha256']:
        raise SystemExit('table payload identity mismatch')
    build_root=a.build_root.resolve() if a.build_root else root/'build'
    build_root.mkdir(parents=True,exist_ok=True)
    out=Path(tempfile.mkdtemp(prefix=a.variant+'.',dir=build_root))
    # Compile only the exact verified bytes. Unlisted headers in caller trees
    # cannot shadow these inputs through quoted-include directory precedence.
    staged={group:out/'closure'/group for group in bases}
    for group,entries in verified.items():
        for name,data in entries.items():
            destination=staged[group]/name;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(data)
    local=out/'closure/scheduler'
    local_files=['scheduler.hpp','scheduler.cpp','tests/scheduler_test.cpp','oracle/oracle.hpp','oracle/oracle.cpp','oracle/MANIFEST.json','oracle/generate.py']
    local_manifest=[]
    for name in local_files:
        data=(root/name).read_bytes();destination=local/name;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(data)
        local_manifest.append({'path':name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
    (out/'COMPILED-SOURCES.json').write_text(json.dumps(local_manifest,indent=2)+'\n')
    test_source=local/'tests/scheduler_test.cpp'
    if a.smoke:
        # Reuse the unchanged canonical snapshot/NoHeap/Pair helpers, without
        # executing the original full-suite entry point or its 48 cases.
        smoke='''#define main scheduler_full_fixture_main
#include "tests/scheduler_test.cpp"
#undef main
int main(int argc,char**argv) {
    CHECK(argc==2); Fixture f(argv[1]);
    for(int rate:{44100,48000,96000}) {
        case_name="portable control consumer binding"; ++cases;
        Pair p(f,rate,true);
        p.parameter(portamentoHalfTime,.17f); p.parameter(glide,.08f);
        p.start(60,80,123); p.block(16);
        p.start(67,110); p.block(16); p.stop(67); p.block(8);
        p.stop(60); p.block(8);
    }
    std::printf("{\\"scope\\":\\"portable-controls-consumer-smoke\\",\\"cases\\":%u,\\"frames\\":%llu,\\"state_snapshots\\":%llu,\\"events\\":%llu,\\"heap_calls_during_actions\\":0,\\"failures\\":0}\\n",cases,frames,snapshots,events);
    return 0;
}
'''
        test_source=local/'tests/consumer_smoke.cpp';test_source.write_text(smoke)
        local_manifest.append({'path':'tests/consumer_smoke.cpp','bytes':len(smoke.encode()),'sha256':hashlib.sha256(smoke.encode()).hexdigest()})
        (out/'COMPILED-SOURCES.json').write_text(json.dumps(local_manifest,indent=2)+'\n')
    staged_tables=out/'public-morphs.f32';staged_tables.write_bytes(tables.read_bytes())
    if hashlib.sha256(staged_tables.read_bytes()).hexdigest()!=expected['sha256']:
        raise SystemExit('table payload changed during staging')
    generated=out/'oracle-binding'
    subprocess.run([__import__('sys').executable,str(local/'oracle/generate.py'),
                    '--kernel-dir',str(staged['master']/'AudioKitSynthOne/DSP/Kernel'),
                    '--output-dir',str(generated)],check=True,stdout=subprocess.DEVNULL)
    for name in ['oracle.hpp','oracle.cpp','MANIFEST.json']:
        if (generated/name).read_bytes()!=(local/'oracle'/name).read_bytes():
            raise SystemExit('actual-source oracle binding mismatch: '+name)
    core=staged['experiment']/'audiokit-4.9.2/AudioKit/Core'
    includes=[local,staged['experiment']/'probe',staged['experiment']/'shared-graph',staged['controls'],core/'Soundpipe',core/'SoundpipeExtension',core/'Soundpipe/lib/dr_wav']
    flags=['-fno-builtin','-ffunction-sections','-fdata-sections','-ffp-contract=off','-fno-fast-math','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-Wno-misleading-indentation']
    flags+=['-O2'] if a.variant=='o2' else ['-Os'] if a.variant=='os' else ['-O1','-g','-fno-omit-frame-pointer','-fsanitize=address,undefined,float-cast-overflow','-fno-sanitize-recover=all']
    inc=['-I'+str(x) for x in includes]
    objects=[]
    commands=[]
    build_env=dict(os.environ)
    # These ambient search paths could inject even nominal system headers ahead
    # of the compiler's trusted system closure. No caller paths are required.
    for name in ['CPATH','C_INCLUDE_PATH','CPLUS_INCLUDE_PATH','OBJC_INCLUDE_PATH']:
        build_env.pop(name,None)
    def command(args):
        commands.append([str(x) for x in args])
        with (out/'commands.json').open('w') as f:json.dump(commands,f,indent=2)
        subprocess.run([str(x) for x in args],check=True,env=build_env)
    for name in ['base','ftbl','adsr','crossfade','osc','fosc','noise','moogladder','butbp','buthp','port','phasor']:
        dest=out/(name+'.o');objects.append(dest)
        command([os.environ.get('CC','cc'),'-std=gnu99','-DNO_LIBSNDFILE']+flags+['-Wno-error']+inc+['-c',core/'Soundpipe/modules'/(name+'.c'),'-o',dest])
    sources=[('morph',core/'SoundpipeExtension/modules/oscmorph2d.c','c'),('voice',staged['experiment']/'probe/portable_voice.cpp','c++'),('parameter_map',staged['controls']/'generated/parameter_map.cpp','c++'),('ports',staged['controls']/'parameter_ports.cpp','c++'),('scheduler',local/'scheduler.cpp','c++'),('oracle',local/'oracle/oracle.cpp','c++'),('test',test_source,'c++')]
    for name,source,lang in sources:
        dest=out/(name+'.o');objects.append(dest)
        command([os.environ.get('CC' if lang=='c' else 'CXX','cc' if lang=='c' else 'c++'),'-std=gnu99' if lang=='c' else '-std=c++11','-DNO_LIBSNDFILE']+flags+(['-Wno-error'] if name in ['morph','voice'] else [])+inc+['-c',source,'-o',dest])
    binary=out/'scheduler_test'
    command([os.environ.get('CXX','c++')]+flags+objects+['-Wl,--gc-sections','-Wl,--wrap=malloc','-Wl,--wrap=calloc','-Wl,--wrap=realloc','-Wl,--wrap=free','-lm','-o',binary])
    env=dict(build_env,ASAN_OPTIONS='detect_leaks=1')
    result=subprocess.run([str(binary),str(staged_tables)],text=True,capture_output=True,env=env)
    (out/'stdout.txt').write_text(result.stdout);(out/'stderr.txt').write_text(result.stderr)
    (out/'exit.txt').write_text(str(result.returncode)+'\n')
    print(result.stdout,end='');print(result.stderr,end='',file=__import__('sys').stderr)
    print('result directory: '+str(out))
    if result.returncode:raise SystemExit(result.returncode)

if __name__=='__main__':main()
