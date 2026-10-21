#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Test packaging against supplied, unchanged DEV artifacts. No compile or device.

Usage: test_package_dev.py --source-repo REPO --validated-result RESULT
The artifact directory is external and is never modified or committed.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source-repo', type=Path, required=True)
parser.add_argument('--validated-result', type=Path, required=True)
parser.add_argument('--host-tools-repo', type=Path)
parser.add_argument('--historical-baseline', action='store_true')
a = parser.parse_args()
source, result = a.source_repo.resolve(), a.validated_result.resolve()
original_record = (result / 'record.json').read_bytes()
record = json.loads(original_record)

def run(argv, rc=0, env=None):
    p = subprocess.run(argv, text=True, capture_output=True, env=env)
    assert p.returncode == rc, (argv, p.returncode, rc, p.stdout, p.stderr)
    return p.stdout + p.stderr

with tempfile.TemporaryDirectory(prefix='distribution-test-') as td:
    base = Path(td)
    kit, zipped = base / 'kit with spaces', base / 'candidate.zip'
    generate = [sys.executable, '-B', str(ROOT/'tools/distribution/package_dev.py'),
                '--source-repo', str(source), '--validated-result', str(result),
                '--output', str(kit), '--zip', str(zipped)]
    if a.host_tools_repo:
        generate[3:3] = ['--host-tools-repo', str(a.host_tools_repo.resolve())]
    if a.historical_baseline:
        generate[3:3] = ['--historical-baseline']
    inside = generate[:-2] + ['--zip', str(kit/'recursive.zip')]
    assert 'ZIP must be outside' in run(inside, 3)
    assert not kit.exists()
    print('PASS recursive ZIP destination refused before output creation')
    template=base/'private-template';template.mkdir();(template/'private-template-marker').write_text('PRIVATE TEST FIXTURE')
    run(generate, env={**os.environ, 'GIT_TEMPLATE_DIR':str(template)})
    assert not (kit/'x0x37/.git/private-template-marker').exists()
    print('PASS host Git template content excluded')
    print('PASS generated exact Elite DEV kit with embedded controller, no rebuild')
    run(generate, 3)
    print('PASS existing kit and ZIP are never overwritten')
    extracted = base / 'extracted'; extracted.mkdir()
    run(['unzip', '-q', str(zipped), '-d', str(extracted)])
    kit = extracted / kit.name
    env = {**os.environ, 'X0X_QUEUE_FILE': str(base/'intentionally-absent-queue')}
    verified = run(['bash', str(kit/'Install.command'), '--verify-only'], env=env)
    assert 'COMBINED OK' in verified and 'DEV CHECKED' in verified and 'QUEUE NOT CHECKED' in verified
    assert not (kit/'.venv').exists()
    print('PASS clean ZIP extraction verifies offline without dependencies or device')
    internal = kit / 'evidence/validated-build'
    assert (internal/'record.json').read_bytes() == original_record
    for name in ['x0x-smk37-elite.fwsc', 'ctl.img', *[x+'.log' for x in record['logs']]]:
        assert (internal/name).read_bytes() == (result/name).read_bytes()
    assert len(list((kit/'firmware').iterdir())) == 1
    print('PASS original raw record/logs/controller preserved, only one user-facing firmware')
    verify = [sys.executable, '-B', str(kit/'verify_combined.py')]
    manifest_path = kit/'MANIFEST.json'; manifest = json.loads(manifest_path.read_text())
    assert manifest['runtime_commit'] == record['commit']
    assert manifest['runtime_rebuild'] is False
    if a.historical_baseline:
        assert manifest['post_install_watch'] == 'manual-only'
        assert manifest['host_tools']['commit'] == subprocess.check_output(
            ['git', '-C', str(a.host_tools_repo), 'rev-parse', 'HEAD'], text=True).strip()
        p = kit/'host-tools/tools/dev_validate.py'; original=p.read_bytes()
        p.write_bytes(original+b'\n# changed\n')
        assert 'host tools identity mismatch' in run(verify,3); p.write_bytes(original)
        print('PASS maintained host tools are separately pinned and tamper-refused')
    for name in manifest['identities']:
        p = kit/name; original = p.read_bytes(); changed = bytearray(original); changed[-1] ^= 1
        p.write_bytes(changed)
        assert 'artifact identity mismatch' in run(verify, 3)
        p.write_bytes(original)
        print('PASS tampered artifact refused:', name)
    p=internal/'build.log'; original=p.read_bytes(); p.write_bytes(original+b'altered')
    assert 'validation log changed' in run(verify,3); p.write_bytes(original)
    print('PASS changed original build log refused')
    p=kit/'firmware/x0x-smk37-elite-with-bt.fwsc'; original=p.read_bytes(); p.unlink()
    run(verify,3); p.write_bytes(original)
    print('PASS missing combined firmware refused')
    original=manifest_path.read_bytes()
    altered={**manifest, 'content_identities': {**manifest['content_identities'], 'app_bin_sha256': '0'*64}}
    manifest_path.write_text(json.dumps(altered))
    assert 'decoded package identity mismatch' in run(verify,3)
    manifest_path.write_bytes(original)
    print('PASS incorrect decoded app identity refused')
    for field,value in [('model','pro'), ('config','rel'), ('runtime_commit','0'*40)]:
        altered={**manifest,field:value}; manifest_path.write_text(json.dumps(altered)); run(verify,3)
        print('PASS invalid distribution metadata refused:',field)
    manifest_path.write_bytes(original)
    p=kit/'x0x37/tools/add_ctl.py'; original=p.read_bytes(); p.write_bytes(original+b'\n# changed\n')
    run(verify,3); p.write_bytes(original)
    print('PASS modified runtime source refused')
    for field,value in [('model','pro'), ('config','rel'), ('controller',False)]:
        bad=base/('bad-'+field); bad.mkdir(); altered={**record,field:value}
        (bad/'record.json').write_text(json.dumps(altered))
        run([sys.executable,'-B',str(ROOT/'tools/distribution/package_dev.py'),
             '--source-repo',str(source),'--validated-result',str(bad),
             '--output',str(base/('reject-'+field))],3)
        print('PASS wrong build record refused:',field)
    assert (result/'record.json').read_bytes() == original_record
    print('PASS external input directory unchanged; no firmware build or device access')
