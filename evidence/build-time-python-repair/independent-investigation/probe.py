#!/usr/bin/env python3
"""Independent scratch edge probes for package enumeration and content walking."""
from pathlib import Path
import ast
import hashlib
import importlib.metadata
import json
import os
import subprocess
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT/'tools'))
import validation_inputs as v

BASE = '6d690e347b2c66d331f0b6da58a390b627ee84b7'
CANDIDATE = 'cadeae04bdc9160cfe15a74427829165e36ba60b'
def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args], text=True).strip()
assert git('rev-parse', 'HEAD') == CANDIDATE
old = subprocess.check_output(['git', '-C', str(ROOT), 'show', BASE+':tools/validation_inputs.py'], text=True)
tree = ast.parse(old)
original = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'content')
ns = {'Path': Path, 'digest': v.digest, 'encoded': v.encoded, 'file_hash': v.file_hash}
exec(compile(ast.Module(body=[original], type_ignores=[]), 'original-content', 'exec'), ns)
original_content = ns['content']
rows = []
def check(name, operation):
    operation()
    rows.append({'case': name, 'result': 'PASS'})

parent = ROOT/'build/independent-investigation'
parent.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(dir=parent) as tmp:
    install = Path(tmp)/'site-packages'; install.mkdir()
    meta = install/'specimen.egg-info'; meta.mkdir()
    (meta/'PKG-INFO').write_text('Name: specimen\nVersion: 2.0\n')
    (meta/'top_level.txt').write_text('namespace\nstandalone\n')
    pkg = install/'namespace'; pkg.mkdir()
    (pkg/'module.py').write_text('raise RuntimeError("must not import")\n')
    (pkg/'native.so').write_bytes(b'extension bytes')
    (pkg/'data.bin').write_bytes(b'package data')
    single = install/'standalone.py'; single.write_text('raise RuntimeError("must not import")\n')
    dist = importlib.metadata.Distribution.at(meta)
    baseline = v.package_files(dist)
    assert set(baseline) == {'namespace/module.py', 'namespace/native.so', 'namespace/data.bin', 'standalone.py'}
    check('namespace_single_module_extension_and_data_without_import', lambda: None)
    for path in (pkg/'module.py', pkg/'native.so', pkg/'data.bin', single):
        data = path.read_bytes(); info = path.stat()
        path.write_bytes(data+b'changed'); os.utime(path, ns=(info.st_atime_ns, info.st_mtime_ns))
        assert v.package_files(dist) != baseline
        path.write_bytes(data); os.utime(path, ns=(info.st_atime_ns, info.st_mtime_ns))
        assert v.package_files(dist) == baseline
    check('all_fallback_byte_mutations_invalidate_with_mtime_preserved', lambda: None)
    (pkg/'in-root-alias').symlink_to(single)
    assert v.package_files(dist)['namespace/in-root-alias'] == sha(single)
    (pkg/'in-root-alias').unlink()
    check('safe_in_root_file_symlink_is_hashed', lambda: None)
    (pkg/'dangling').symlink_to(pkg/'missing')
    try:
        v.package_files(dist)
    except FileNotFoundError:
        pass
    else:
        raise AssertionError('dangling package link was accepted')
    (pkg/'dangling').unlink()
    check('dangling_fallback_link_fails_closed', lambda: None)
    (meta/'RECORD').write_text('namespace/module.py,,\nnamespace/native.so,,\nnamespace/data.bin,,\nstandalone.py,,\n')
    assert v.package_files(dist) == baseline
    (meta/'top_level.txt').write_text('../unsafe\n')
    assert v.package_files(dist) == baseline
    check('valid_normal_manifest_remains_authoritative', lambda: None)
    (meta/'RECORD').write_text('namespace/missing.py,,\n')
    try:
        v.package_files(dist)
    except FileNotFoundError:
        pass
    else:
        raise AssertionError('missing manifest input was accepted')
    check('missing_manifest_file_fails_closed', lambda: None)
    (meta/'RECORD').unlink()
    try:
        v.package_files(dist)
    except ValueError:
        pass
    else:
        raise AssertionError('unsafe top level was accepted')
    check('fallback_does_not_use_unsafe_module_declaration', lambda: None)

    root = Path(tmp)/'walk'; root.mkdir()
    (root/'payload').write_bytes(b'bytes')
    (root/'executable').write_bytes(b'bytes'); (root/'executable').chmod(0o751)
    sub = root/'sub'; sub.mkdir(); (sub/'data').write_bytes(b'data')
    (root/'dir-alias').symlink_to(sub)
    (sub/'file-alias').symlink_to(root/'payload')
    for name in ('.git', '__pycache__', 'custom'):
        (sub/name).symlink_to(root/'does-not-exist')
    for path in (root, sub, root/'payload', root/'dir-alias', sub/'file-alias'):
        assert original_content(path, exclude=('custom',)) == v.content(path, exclude=('custom',))
    check('walker_nested_aliases_modes_and_exclusions_match_original', lambda: None)
    (sub/'loop').symlink_to(root)
    for function in (original_content, v.content):
        try:
            function(root, exclude=('custom',))
        except ValueError as error:
            assert 'symlink cycle' in str(error)
        else:
            raise AssertionError('walker cycle accepted')
    (sub/'loop').unlink()
    check('both_walkers_reject_cycles', lambda: None)

    # Direct gate capture CLI must emit no usable identity for an installed but
    # unenumerable package. All repositories and dependencies here are scratch.
    repo = Path(tmp)/'repo'; repo.mkdir()
    subprocess.run(['git', 'init', '-q', str(repo)], check=True)
    (repo/'source.c').write_text('fixture')
    subprocess.run(['git', '-C', str(repo), 'add', '.'], check=True)
    subprocess.run(['git', '-C', str(repo), '-c', 'user.name=fixture', '-c', 'user.email=fixture@fixture', 'commit', '-qm', 'fixture'], check=True)
    sdk = Path(tmp)/'sdk'; sdk.mkdir(); (sdk/'input').write_text('sdk')
    tc = Path(tmp)/'toolchain'; tc.mkdir(); (tc/'input').write_text('toolchain')
    refs = Path(tmp)/'refs'; refs.mkdir()
    hook = Path(tmp)/'hook'; hook.write_text('hook')
    argv = [sys.executable, '-B', str(ROOT/'tools/validation_inputs.py'), 'capture',
            '--repo', str(repo), '--commit', 'HEAD', '--toolchain', str(tc), '--sdk', str(sdk),
            '--refs', str(refs), '--pins', '', '--hook', str(hook), '--runner', str(ROOT/'tools/gate.sh')]
    env = dict(os.environ, PYTHONPATH=str(install), X0X_VALIDATION_PYTHON_PACKAGES='specimen', CC='cc', CXX='c++')
    result = subprocess.run(argv, env=env, capture_output=True, text=True)
    assert result.returncode == 2 and not result.stdout.strip(), result
    assert 'cannot enumerate Python package' in result.stderr
    rows.append({'case': 'gate_capture_cli_refuses_unenumerable_installed_package', 'result': 'PASS',
                 'argv': argv, 'returncode': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr,
                 'environment_overrides': {'PYTHONPATH': str(install), 'X0X_VALIDATION_PYTHON_PACKAGES': 'specimen', 'CC': 'cc', 'CXX': 'c++'}})

inputs = json.loads((OUT.parent/'inputs.json').read_text())
assert all(sha(ROOT/path) == expected for path, expected in inputs['source_hashes'].items())
assert all(sha(ROOT/path) == expected for path, expected in inputs['unchanged_consumers_and_generators'].items())
checks = json.loads((OUT.parent/'checks.json').read_text())
for checkrow in checks['checks']:
    assert checkrow['returncode'] == 0
    assert sha(OUT.parent/checkrow['log']) == checkrow['sha256']
proof = json.loads((OUT.parent/'package-proof.log').read_text())
assert proof['distribution_files_is_none'] and proof['restored']
assert proof['runtime_before']['python_packages']['pillow']['sha256'] != v.digest(v.encoded({}))
for mutation in proof['mutations']:
    assert mutation['before'] != mutation['after']
    assert mutation['runtime_after']['python_packages']['pillow'] != proof['runtime_before']['python_packages']['pillow']
walk = json.loads((OUT.parent/'walker-proof.json').read_text())
assert walk['equal'] and all(row['original'] == row['direct'] for row in walk['results'])
def verify_ast(revision):
    source = subprocess.check_output(['git', '-C', str(ROOT), 'show', revision+':tools/dev_validate.py'], text=True)
    return ast.dump(next(n for n in ast.parse(source).body if isinstance(n, ast.FunctionDef) and n.name == 'verify'))
assert verify_ast(CANDIDATE) == verify_ast('6affed8')
(OUT/'probe-result.json').write_text(json.dumps({'candidate': CANDIDATE, 'source_unchanged': True,
    'source_hashes': inputs['source_hashes'], 'unchanged_consumers_and_generators': inputs['unchanged_consumers_and_generators'],
    'cases': rows, 'reused_checks_verified': checks['checks'],
    'worker_proof_verified': {'log': str((OUT.parent/'package-proof.log').relative_to(ROOT)), 'sha256': sha(OUT.parent/'package-proof.log'), 'mutations': len(proof['mutations'])},
    'walker_sdk_toolchain_proof_verified': {'file': str((OUT.parent/'walker-proof.json').relative_to(ROOT)), 'sha256': sha(OUT.parent/'walker-proof.json')},
    'verify_ast_matches_audit': True}, indent=2)+'\n')
print('PASS:', len(rows), 'independent edge probes; saved source/log hashes and actual worker/walker proofs verified.')
