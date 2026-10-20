"""Stage-3 read-only source checks; scratch binaries and scoped evidence only."""
from pathlib import Path
import subprocess, json, hashlib, datetime

root = Path(__file__).resolve().parents[3]
out = Path(__file__).resolve().parent
checks = [
    ('diag-decoder', ['python3','tests/ble_central_diag_test.py'], 'old and versioned diagnostic decoding; preserve old field positions'),
    ('diag-bounds', ['python3','tests/diag_size_test.py'], 'ED_DIAG word/output bounds and unchanged INLOG fit'),
    ('receive', ['sh','tests/host/ble_midi_receive_test.sh'], 'production receive lifecycle and bounded trace under address/undefined sanitizers'),
    ('capture', ['python3','tests/host/ble_capture_replay_test.py'], 'recorded payload replay with simulated whole/fragmented HCI and separately resegmented MTU23'),
]
results = []
for name, argv, reason in checks:
    run = subprocess.run(argv, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (out/(name+'.log')).write_text(run.stdout)
    results.append(dict(name=name, argv=argv, reason=reason, exit_code=run.returncode, result='PASS' if run.returncode==0 else 'FAIL', log=name+'.log'))
    print(name, run.returncode, run.stdout[-2500:], flush=True)
identity = json.loads((root/'evidence/smc-stage2/CHECKS.json').read_text())['source_sha256']
hashes = {}
for name, expected in identity.items():
    if name in ('firmware/src/app/editor.c','firmware/src/ble.c','firmware/src/ble/ble_midi_central.c','firmware/src/ble/ble_midi_central.h','tools/ble_autotest.py','tests/ble_central_diag_test.py','tests/host/ble_midi_receive_test.c'):
        actual = hashlib.sha256((root/name).read_bytes()).hexdigest()
        hashes[name] = dict(expected=expected, actual=actual, match=actual==expected)
report = dict(commit=subprocess.check_output(['git','-c','safe.directory='+str(root),'rev-parse','HEAD'],cwd=root,text=True).strip(), time_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), checks=results, source_hashes=hashes, hardware='NOT TESTED; live all-input silence unresolved', limits=['Replay retains notification payload bytes; HCI framing and connected state are simulated.','The MTU23 case changes segmentation and is not exact wire replay.','No target build, controller build, full gate, install or push.'])
(out/'checks.json').write_text(json.dumps(report, indent=2)+'\n')
assert all(x['match'] for x in hashes.values())
assert all(x['exit_code']==0 for x in results)
