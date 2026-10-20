#!/usr/bin/env python3
"""Independent clean-worktree asset proof. Host controls disposable checked worktrees.

Each worker runs in a separate pinned Linux container. Source is read-only,
outputs are tmpfs, and a unique disposable cache lives under the Git common directory.
No production file is copied over a checkout and no firmware is built.
"""
from pathlib import Path
import hashlib, json, os, shutil, subprocess, sys, tempfile, time, uuid

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
ADMIN = Path('/Users/mac/Documents/x0x37/asset_reuse_selector_repair')
CANDIDATE = '746ece6936a456ef963c758da659c07a801cec37'
IMAGE = 'sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1'
NAMES = ('felucca_font.h', 'x0x_drum_samples.h', 'x0x_drum_tables.h', 'x0x_break_bank.h')

def git(*args):
    return subprocess.check_output(['git', '-C', str(ADMIN), *args], text=True).strip()

def worker():
    root, mode, namespace = Path(sys.argv[2]), sys.argv[3], sys.argv[4]
    os.chdir(root)
    sys.path.insert(0, str(root/'tools'))
    import asset_cache, build
    default = asset_cache.cache_directory(root)
    store = default / namespace
    asset_cache.cache_directory = lambda source: store
    if mode in ('warm', 'fresh'):
        def forbidden(*args, **kwargs):
            raise AssertionError('Persistent warm reuse restarted discovery')
        asset_cache.run_checked = forbidden
    start = time.monotonic()
    build.generate()
    report = {'seconds': time.monotonic()-start, 'cache_default': str(default),
              'cache_selected': str(store), 'uid': os.getuid(), 'gid': os.getgid(),
              'hashes': {name: hashlib.sha256((root/'build/gen'/name).read_bytes()).hexdigest() for name in NAMES}}
    print('WORKER_RESULT='+json.dumps(report), flush=True)

def main():
    original = {p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in ('tools/asset_cache.py','tools/build.py','tests/host/test_asset_cache.py')}
    common = Path(git('rev-parse','--git-common-dir')).resolve()
    namespace = 'stage3-selector-test-'+uuid.uuid4().hex
    cache = common/'x0x37-asset-cache/v1'/namespace
    temp = Path(tempfile.mkdtemp(prefix='scratch-',dir=OUT))
    trees = [temp/'first', temp/'second']
    report = {'candidate': CANDIDATE, 'tree': git('rev-parse',CANDIDATE+'^{tree}'), 'image':IMAGE,
              'platform':'linux/amd64','network':'none','source_sha256':original,'commands':[], 'runs':{}, 'worktrees':{},
              'cache_namespace':str(cache), 'failures':[]}
    try:
        for tree in trees:
            command = ['git','-C',str(ADMIN),'worktree','add','--detach',str(tree),CANDIDATE]
            report['commands'].append(command)
            subprocess.run(command, check=True)
            (tree/'build').mkdir(exist_ok=True)  # Mountpoint only; ignored and empty on host.
            head = subprocess.check_output(['git','-C',str(tree),'rev-parse','HEAD'],text=True).strip()
            clean = subprocess.check_output(['git','-C',str(tree),'status','--porcelain'],text=True).strip()
            assert head == CANDIDATE and not clean, (head,clean)
            report['worktrees'][str(tree)]={'head':head,'before_status':clean}
        expected = None
        for mode in ('disabled','cold','warm','fresh'):
            tree = trees[1] if mode == 'fresh' else trees[0]
            command = ['docker','run','--rm','--platform','linux/amd64','--network','none','--user','501:20',
                '-e','X0X_ASSET_CACHE='+('0' if mode == 'disabled' else '1'),
                '-v',str(ROOT)+':'+str(ROOT)+':ro', '-v',str(tree)+':'+str(tree)+':ro',
                '-v',str(common)+':'+str(common)+':rw',
                '--tmpfs',str(tree/'build')+':rw,exec,uid=501,gid=20,mode=755', '-w',str(tree),IMAGE,
                'python3','-u','-B',str(Path(__file__).resolve()),'--worker',str(tree),mode,namespace]
            report['commands'].append(command)
            start = time.monotonic()
            result = subprocess.run(command,capture_output=True,text=True)
            print('$ '+repr(command),flush=True)
            print(result.stdout,end='',flush=True)
            print(result.stderr,end='',flush=True)
            assert result.returncode == 0, (mode,result.returncode)
            run = json.loads(next(line.split('=',1)[1] for line in result.stdout.splitlines() if line.startswith('WORKER_RESULT=')))
            run['container_wall_seconds']=time.monotonic()-start
            run['hits']=[line for line in result.stdout.splitlines() if line.startswith('asset cache:')]
            assert len(run['hits'])==3 and all(s.endswith(' hit' if mode in ('warm','fresh') else ' miss') for s in run['hits']), run
            if expected is None: expected = run['hashes']
            assert run['hashes']==expected,mode
            report['runs'][mode]=run
        assert len({r['cache_default'] for r in report['runs'].values()})==1
        original_expected=json.loads((ADMIN/'evidence/asset-selector-repair/timing.json').read_text())['runs']['disabled']['hashes']
        assert expected==original_expected
        for tree in trees:
            clean=subprocess.check_output(['git','-C',str(tree),'status','--porcelain'],text=True).strip()
            assert not clean,clean
            report['worktrees'][str(tree)]['after_status']=clean
        report['warm_speedup']=report['runs']['disabled']['seconds']/report['runs']['warm']['seconds']
        report['seconds_saved']=report['runs']['disabled']['seconds']-report['runs']['warm']['seconds']
        report['result']='PASS'
        report['limits']='Asset generation only. Timing excludes container startup and includes separate worker processes; no whole-build speed claim.'
    except BaseException as error:
        report['result']='FAIL'
        report['failures'].append(repr(error))
        raise
    finally:
        report['source_unchanged'] = original == {p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in original}
        (OUT/'process-result.json').write_text(json.dumps(report,indent=2)+'\n')
        for tree in reversed(trees):
            if tree.exists(): subprocess.run(['git','-C',str(ADMIN),'worktree','remove',str(tree)],check=True)
        shutil.rmtree(temp)
        if cache.exists(): shutil.rmtree(cache)
        print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':
    if len(sys.argv)>1 and sys.argv[1]=='--worker': worker()
    else: main()
