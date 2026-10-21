#!/usr/bin/env python3
"""Check the isolated fixture repair and two source-negative controls. No target build."""
from pathlib import Path
import os
import shutil
import shlex
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
REF = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else HERE.parent.parent / 'reference-smc-integration'
OUT = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else HERE.parent / 'validation' / 'independent-review'
OUT.mkdir(parents=True, exist_ok=True)
original = (REF / 'tests/host/smc_clip_visibility_test.c').read_text()
repaired = (HERE / 'smc_clip_visibility_test.c').read_text()
original_mixer = (REF / 'firmware/src/app/smc_mixer.c').read_text()
script = (REF / 'tests/host/smc_clip_visibility_test.sh').read_text()
assert '-Wall -Wextra -Werror' in script

with tempfile.TemporaryDirectory() as scratch:
    scratch = Path(scratch)
    tests = scratch / 'tests/host'
    tests.mkdir(parents=True)
    (tests / 'smc_clip_visibility_test.sh').write_text(script)
    src = scratch / 'firmware/src'
    (src / 'app').mkdir(parents=True)
    for name in ('app/main_fm1.c', 'app/smc_protocol.h', 'app/smk_clip_keys.h', 'x0x_cold.h'):
        shutil.copyfile(REF / 'firmware/src' / name, src / name)
    (tests / 'smc_clip_visibility_test.c').write_text(original)
    apply = subprocess.run(['patch', '-p1'], cwd=scratch, input=(HERE / 'fixture.patch').read_text(), text=True, capture_output=True)
    assert apply.returncode == 0, apply.stderr
    assert (tests / 'smc_clip_visibility_test.c').read_text() == repaired
    (OUT / 'patch-apply.log').write_text(apply.stdout + 'Exact patched fixture matches reviewed source.\n')

    def run(label, fixture, mixer, expected_pass, extra_env=None):
        (tests / 'smc_clip_visibility_test.c').write_text(fixture)
        (src / 'app/smc_mixer.c').write_text(mixer)
        result = subprocess.run(['sh', str(tests / 'smc_clip_visibility_test.sh')], cwd=scratch, text=True, capture_output=True, env={**os.environ, 'SRC': str(src), **(extra_env or {})})
        output = result.stdout + result.stderr
        (OUT / (label + '.log')).write_text(output)
        assert (result.returncode == 0) == expected_pass, (label, result.returncode, output)
        print(f'{label}: exit={result.returncode}; PASS lines={output.count(chr(10)+"PASS ") + output.startswith("PASS ")}; FAIL lines={output.count(chr(10)+"FAIL ") + output.startswith("FAIL ")}')
        return output

    baseline = run('baseline', original, original_mixer, False)
    assert 'unused-function' in baseline and 'mark_dirty' in baseline
    old = 'static void mark_dirty(void) { dirty++; }\n'
    assert original.count(old) == 1
    remove_only = run('remove-only', original.replace(old, ''), original_mixer, False)
    assert remove_only.count('FAIL ') == 9
    assert all('visible stage edits its first field' in line for line in remove_only.splitlines() if line.startswith('FAIL '))
    positive = run('repaired', repaired, original_mixer, True)
    assert positive.count('PASS ') == 138
    compiler = scratch / 'cc-test-profile.sh'
    compiler.write_text('#!/bin/sh\nexec ' + shlex.quote(os.environ.get('CC', 'cc')) + ' -DX0X_SMC_VENDOR_TEST=1 \"$@\"\n')
    compiler.chmod(0o700)
    test_profile = run('repaired-test-profile', repaired, original_mixer, True, {'CC': str(compiler)})
    assert test_profile.count('PASS ') == 138

    # If covered stages regain their knobs, unchanged visibility assertions must fail.
    guard = 'if (ui.overlay == O_NONE && smk_ck_smc_turn(n, delta))'
    assert original_mixer.count(guard) == 1
    hidden = run('negative-hidden-edits', repaired, original_mixer.replace(guard, 'if (smk_ck_smc_turn(n, delta))'), False)
    assert any('menu owns encoders without editing covered clip fields' in line for line in hidden.splitlines() if line.startswith('FAIL '))

    # Retaining blocked events is wrong even if no parameter changes under the overlay.
    poll = 'void smc_poll(void)\n{\n'
    assert original_mixer.count(poll) == 1
    replay = run('negative-replay', repaired, original_mixer.replace(poll, poll + '    if (ui.help || ui.overlay == O_ASK) return; /* negative control only */\n'), False)
    assert any('closing the overlay does not replay turns' in line for line in replay.splitlines() if line.startswith('FAIL '))
print('Review controls passed: original failure, 9 stale assertions, repaired success, hidden-edit and replay sensitivity.')
