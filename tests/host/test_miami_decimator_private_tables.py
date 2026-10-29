#!/usr/bin/env python3
"""Explicit private-manifest argument; no network/original binary/production activity."""
import importlib.util,pathlib,tempfile,sys,json,hashlib,struct
root=pathlib.Path(__file__).resolve().parents[2];source=pathlib.Path(sys.argv[1])
spec=importlib.util.spec_from_file_location('g',root/'tools/gen_miami_decimator_private_tables.py');g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d);out=p/'tables.h';g.generate(source,out)
 assert out.read_bytes()==(root/'experiments/wobble/miami_decimator_tables_private.h').read_bytes()
 before=out.read_bytes();bad=p/'bad.json';j=json.loads(source.read_text());j['functional_tables']['frequency']['values'][-1]-=1;bad.write_text(json.dumps(j))
 try:g.generate(bad,out)
 except ValueError:pass
 else:raise AssertionError('modified input admitted')
 assert out.read_bytes()==before
j=json.loads(source.read_text())
for name,fmt in [('frequency','H'),('wet','h')]:
 t=j['functional_tables'][name];assert len(t['values'])==128
 assert hashlib.sha256(struct.pack('<128'+fmt,*t['values'])).hexdigest()==t['source_raw_sha256']
print('PASS256 exact transferred coefficients, source/table hashes, deterministic generation, rejection/nonmutation')
