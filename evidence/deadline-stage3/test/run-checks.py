"""Record scoped host checks and source-negative controls in scratch trees."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import tempfile

R = Path(__file__).resolve().parents[3]
E = Path(__file__).resolve().parent
records = []
FLAGS = ['cc', '-std=c99', '-g', '-O1', '-Wall', '-Wextra', '-Werror',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all']

def run(name, argv, cwd=R, expected=0, assertion=False):
    p = subprocess.run([str(a) for a in argv], cwd=cwd, capture_output=True, text=True)
    text = p.stdout + p.stderr
    (E / (name + '.log')).write_text(text)
    sanitizer_error = any(x in text for x in ['ERROR: AddressSanitizer', 'runtime error:', 'UndefinedBehaviorSanitizer:'])
    ok = p.returncode == expected and (not assertion or 'Assertion failed' in text) and not sanitizer_error
    records.append(dict(name=name, argv=[str(a) for a in argv], cwd=str(cwd),
                        returncode=p.returncode, expected=expected, result='PASS' if ok else 'FAIL',
                        log=name + '.log', sanitizer_error=sanitizer_error))
    (E / 'host-checks.json').write_text(json.dumps(records, indent=2) + '\n')
    assert ok, (name, text)

def compile_probe(tree, name, relative):
    binary = tree / name
    run(tree.name + '-' + name + '-compile', FLAGS + [relative,
        'firmware/src/ble/ble_midi_central.c', '-o', binary], tree)
    return binary

run('receive', ['sh', 'tests/host/ble_midi_receive_test.sh'])
run('diag', ['python3', '-B', 'tests/ble_central_diag_test.py'])
with tempfile.TemporaryDirectory(prefix='deadline-controls-') as temp:
    trees = {}
    for label, revision in [('abort01-broken', '28d0fc230f60bc823c1e5e73d3aed61e68a8e32f'),
                            ('deadline-broken', '05eff604cb3f136db71727a2aeeea785cab73b99'),
                            ('repaired', None), ('guard-only', None)]:
        tree = Path(temp) / label; trees[label] = tree
        for folder in ['tests/host', 'evidence/session-stage3', 'evidence/abort-stage3/test',
                       'evidence/abort-stage3/investigation']:
            shutil.copytree(R / folder, tree / folder)
        (tree / 'firmware/src/ble').mkdir(parents=True)
        for relative in ['firmware/src/ble/ble_midi_central.c',
                         'firmware/src/ble/ble_midi_central.h', 'firmware/src/ble.c']:
            data = subprocess.check_output(['git', 'show', revision + ':' + relative], cwd=R) if revision else (R / relative).read_bytes()
            (tree / relative).write_bytes(data)
        if label == 'guard-only':
            (tree / 'firmware/src/ble.c').write_bytes(subprocess.check_output(
                ['git', 'show', '05eff60:firmware/src/ble.c'], cwd=R))
    for label in ['abort01-broken', 'repaired']:
        tree = trees[label]; broken = label == 'abort01-broken'
        for name, source, cases in [
            ('public-abort', 'tests/host/ble_midi_abort_test.c', ['overflow', 'timeout']),
            ('saved-investigation', 'evidence/session-stage3/investigation-abort-probe.c', ['overflow', 'timeout']),
            ('saved-test', 'evidence/session-stage3/test-busy-disconnect.c', [None])]:
            binary = compile_probe(tree, name, source)
            for case in cases:
                expected = (1 if name == 'saved-test' else -6) if broken else 0
                run(label + '-' + name + '-' + str(case), [binary] + ([case] if case else []),
                    tree, expected, broken and name != 'saved-test')
    for label in ['deadline-broken', 'repaired']:
        tree = trees[label]; broken = label == 'deadline-broken'
        binary = compile_probe(tree, 'saved-stop', 'evidence/abort-stage3/test/stop-deadline.c')
        run(label + '-saved-stop', [binary], tree, -6 if broken else 0, broken)
        binary = compile_probe(tree, 'saved-deadline', 'evidence/abort-stage3/investigation/deadline-stop-probe.c')
        for path in ['stop', 'service', 'cancel']:
            for when in ['exact', 'after']:
                for wrap in [False, True]:
                    # The saved service probe intentionally retains the stale caller order.
                    # The production service test below checks the actual repaired caller.
                    fail = broken or path == 'service'
                    run(label + '-saved-' + path + '-' + when + ('-wrap' if wrap else ''),
                        [binary, path, when] + (['wrap'] if wrap else []), tree,
                        -6 if fail else 0, fail)
        for case in ['stop', 'service', 'cancel']:
            run(label + '-public-' + case, ['python3', '-B', 'tests/host/ble_midi_deadline_test.py', case],
                tree, 1 if broken else 0, broken)
    run('guard-only-service-negative', ['python3', '-B', 'tests/host/ble_midi_deadline_test.py', 'service'],
        trees['guard-only'], 1, True)
run('diff', ['git', 'diff', '--check'])
identity = {str(p.relative_to(R)): hashlib.sha256(p.read_bytes()).hexdigest()
            for folder in ['firmware', 'tests', 'tools', 'ble/ctl', 'host']
            for p in sorted((R / folder).rglob('*'))
            if p.is_file() and '.git' not in p.parts and '__pycache__' not in p.parts}
(E / 'source-inputs.json').write_text(json.dumps(identity, indent=2) + '\n')
print('Scoped host checks and expected source-negative controls: PASS')
