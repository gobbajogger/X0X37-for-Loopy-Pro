#!/usr/bin/env python3
"""Independent stage3 checks. Run in the pinned Linux worker as user 501:20.

Use only this evidence folder and disposable build/scratch inputs. No firmware is built.
The worker mode starts a new interpreter for each cache operation.
"""
from pathlib import Path
import hashlib
import json
import os
import py_compile
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
CANDIDATE = 'bb953dfaa086c44e57382e9d82fe9e0a2714f965'
NAMES = ('felucca_font.h', 'x0x_drum_samples.h', 'x0x_drum_tables.h', 'x0x_break_bank.h')

def hashes(gen, names=NAMES):
    return {n: hashlib.sha256((gen / n).read_bytes()).hexdigest() for n in names}

def worker():
    root, store = Path(sys.argv[2]), Path(sys.argv[3])
    mode = sys.argv[4]
    os.chdir(root)
    sys.path.insert(0, str(root / 'tools'))
    import asset_cache
    if mode == 'deny-discovery':
        def unexpected(*args, **kwargs):
            raise AssertionError('persistent inventory unexpectedly rediscovered dependencies')
        asset_cache.run_checked = unexpected
    if sys.argv[5] == 'all':
        import build
        asset_cache.cache_directory = lambda root: store
        build.generate()
        names = NAMES
    else:
        hit = asset_cache.generate_asset('drums', root, root / 'build/gen', store)
        print('WORKER_HIT=' + str(hit))
        names = NAMES[1:3]
    print('WORKER_HASHES=' + json.dumps(hashes(root / 'build/gen', names)))

def run_worker(root, store, mode='normal', family='all'):
    command = [sys.executable, '-u', '-B', str(Path(__file__).resolve()), '--worker', str(root), str(store), mode, family]
    start = time.monotonic()
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print('$ ' + repr(command), flush=True)
    print(result.stdout, end='', flush=True)
    assert result.returncode == 0, result.returncode
    output = next(line for line in result.stdout.splitlines() if line.startswith('WORKER_HASHES='))
    return {'seconds': time.monotonic() - start, 'hashes': json.loads(output.split('=', 1)[1]),
            'hits': [line for line in result.stdout.splitlines() if line.startswith('asset cache:')],
            'command': command}

def main():
    assert sys.platform == 'linux'
    assert subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip() == CANDIDATE
    original = hashes(ROOT / 'tools', ('asset_cache.py', 'build.py'))
    report = {'candidate': CANDIDATE, 'image': os.environ['STAGE3_IMAGE'], 'uid': os.getuid(), 'checks': {}}
    expected = json.loads((ROOT / 'evidence/asset-reuse-stage2/timing.json').read_text())['outputs']
    build = ROOT / 'build'
    build.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='stage3-process-', dir=build) as temp:
        base = Path(temp)
        fresh = base / 'fresh'
        store = base / 'cache'
        # The checked commit is used directly. No copied or modified production file overlays.
        subprocess.run(['git', '-C', str(ROOT), 'worktree', 'add', '--detach', str(fresh), CANDIDATE], check=True)
        try:
            cold = run_worker(fresh, store)
            assert all(line.endswith(' miss') for line in cold['hits']), cold
            assert cold['hashes'] == expected
            shutil.rmtree(fresh / 'build/gen')
            warm = run_worker(fresh, store, 'deny-discovery')
            assert len(warm['hits']) == 3 and all(line.endswith(' hit') for line in warm['hits']), warm
            assert warm['hashes'] == expected
            report['checks']['real_build_cold_warm_processes'] = {'cold': cold, 'warm': warm,
                'risk': 'A new interpreter must reuse verified inventories and preserve all four real generated headers.'}
            # A second clean worktree must reuse the first worktree's normalized inventories.
            other = base / 'second'
            subprocess.run(['git', '-C', str(ROOT), 'worktree', 'add', '--detach', str(other), CANDIDATE], check=True)
            try:
                fresh_run = run_worker(other, store, 'deny-discovery')
                assert len(fresh_run['hits']) == 3 and all(line.endswith(' hit') for line in fresh_run['hits']), fresh_run
                assert fresh_run['hashes'] == expected
                assert not subprocess.check_output(['git', '-C', str(other), 'status', '--porcelain'], text=True).strip()
                report['checks']['fresh_clean_checked_commit'] = fresh_run
            finally:
                subprocess.run(['git', '-C', str(ROOT), 'worktree', 'remove', '--force', str(other)], check=True)
        finally:
            subprocess.run(['git', '-C', str(ROOT), 'worktree', 'remove', '--force', str(fresh)], check=True)
        fixture = base / 'fixture'
        (fixture / 'tools').mkdir(parents=True)
        (fixture / 'assets/909').mkdir(parents=True)
        shutil.copyfile(ROOT / 'tools/asset_cache.py', fixture / 'tools/asset_cache.py')
        (fixture / 'tools/build.py').write_text('# scratch fixture\n')
        (fixture / 'tools/sampleio.py').write_text('# scratch helper\n')
        runtime = fixture / 'tools/fixture_runtime.py'
        runtime.write_text('TOKEN="old"\n')
        (fixture / 'assets/909/input.wav').write_bytes(b'fixture audio')
        (fixture / 'tools/gen_drum_samples.py').write_text(
            "from pathlib import Path\nimport fixture_runtime, sampleio\nSAMPLES=[('sample','input.wav')]\n"
            "if __name__=='__main__':\n import sys\n p=Path(sys.argv[1])\n p.write_text(fixture_runtime.TOKEN)\n"
            " (p.parent/'x0x_drum_tables.h').write_text('table:'+fixture_runtime.TOKEN)\n")
        fstore = base / 'fixture-cache'
        first = run_worker(fixture, fstore, family='drums')
        assert first['hits'] == ['asset cache: drums miss']
        again = run_worker(fixture, fstore, 'deny-discovery', 'drums')
        assert again['hits'] == ['asset cache: drums hit']
        py_compile.compile(str(runtime), doraise=True)
        stamp = runtime.stat()
        runtime.write_text('TOKEN="new"\n')
        os.utime(runtime, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
        changed = run_worker(fixture, fstore, family='drums')
        assert changed['hits'] == ['asset cache: drums miss']
        assert changed['hashes'] != first['hashes']
        assert (fixture / 'build/gen/x0x_drum_samples.h').read_text() == 'new'
        after = run_worker(fixture, fstore, 'deny-discovery', 'drums')
        assert after['hits'] == ['asset cache: drums hit'] and after['hashes'] == changed['hashes']
        report['checks']['same_mtime_runtime_stale_pyc_across_processes'] = {
            'first': first, 'again': again, 'changed': changed, 'after': after,
            'risk': 'Persistent discovery and timestamp-valid stale bytecode must not hide changed imported runtime bytes.'}
        # Use genuine permission errors, rather than the stage2 file-path blocker.
        os.chmod(fstore, 0o555)
        for folder in fstore.rglob('*'):
            if folder.is_dir():
                os.chmod(folder, 0o555)
        runtime.write_text('TOKEN="end"\n')
        try:
            unwritable = run_worker(fixture, fstore, family='drums')
            assert unwritable['hits'] == ['asset cache: drums miss']
            assert (fixture / 'build/gen/x0x_drum_samples.h').read_text() == 'end'
            report['checks']['real_unwritable_directory_fallback'] = unwritable
        finally:
            os.chmod(fstore, 0o755)
            for folder in fstore.rglob('*'):
                if folder.is_dir():
                    os.chmod(folder, 0o755)
    assert hashes(ROOT / 'tools', ('asset_cache.py', 'build.py')) == original
    report['result'] = 'PASS'
    report['source_unchanged'] = original
    report['reused_stage2_evidence'] = ['Corrupt/missing/partial/wrong-key entries, generator failure/signal, parent interruption, concurrent publication, fallback input mutation: exact candidate and pinned environment match.']
    report['limits'] = ['Asset generation and cache boundaries only; no firmware, device, gate or end-to-end validation timing.']
    (OUT / 'process-result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--worker':
        worker()
    else:
        main()
