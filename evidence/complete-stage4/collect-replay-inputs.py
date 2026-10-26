"""Collect all compiler dependencies for the saved combined replay.
The compiler's multi-source -MF output contains only the final unit.
Use -MM on every unit to record all selected production headers and sources.
This command does not repeat the replay.
"""
from pathlib import Path
import hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=Path(__file__).resolve().parent
record=json.loads((OUT/'replay.json').read_text())
argv=record['compile']
sources=[s for s in argv if s.endswith('.c')]
flags=['/usr/bin/cc','-MM','-DX0X_HOST','-D_DARWIN_C_SOURCE','-Ifirmware/src','-Ifirmware/src/dsp','-Ifirmware/hal','-Ibuild/gen']
raw=subprocess.check_output(flags+sources,cwd=ROOT,text=True)
(OUT/'replay-all.d').write_text(raw)
inputs={}
for row in raw.replace('\\\n',' ').splitlines():
    for value in row.split(':',1)[1].split():
        p=Path(value);p=p if p.is_absolute() else ROOT/p
        inputs[str(p.relative_to(ROOT)) if p.is_relative_to(ROOT) else str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
prior=ROOT.parent/'smc_working_source_regression_evidence_20261021'
for p in [Path(__file__),OUT/'replay-run.py',prior/'compare_startup.py',prior/'working-source-store.bin',ROOT/'cloud-handoff/2026-10-20/smc_filter_fix/evidence/smc-raw-ble-retry.json',ROOT/'build/complete-stage4/payloads.bin',Path('/usr/bin/cc')]:
    inputs[str(p.relative_to(ROOT)) if p.is_relative_to(ROOT) else str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
(OUT/'replay-inputs.json').write_text(json.dumps(inputs,indent=2)+'\n')
print(f'{len(inputs)} complete replay inputs are recorded.')
