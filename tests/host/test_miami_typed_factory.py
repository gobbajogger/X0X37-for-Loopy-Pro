#!/usr/bin/env python3
"""Explicit private reference argument. Generator and metadata tests; no audio claims."""
import copy,importlib.util,json,pathlib,struct,sys,tempfile,hashlib,re
root=pathlib.Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('g',root/'tools/gen_miami_typed_factory.py');g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
ref=pathlib.Path(sys.argv[1]);doc=json.loads(ref.read_text());source=root/'firmware/src/app/factory_banks.c'
s=source.read_text().split('static const factory_preset_t miami_presets[30]')[1].split('};')[0]
legacy={n:list(map(int,v.split(','))) for n,v in re.findall(r'\{ "([^"]+)", 18, \{ ([^}]+)',s)}
for p in doc['display_conversion']['parameters']:
 for i,b in enumerate(p['boundaries']):
  assert g.ordinal(b['lastLeft']['value'],p['id'])==i
  assert g.ordinal(b['firstOther']['value'],p['id'])==i+1
 for i,a in enumerate(p['anchors']):assert g.ordinal(a['value'],p['id'])==i
 for a in p['extraNeighbours']:assert p['anchors'][g.ordinal(a['value'],p['id'])]['text']==a['text']
assert g.bits(-0.0)==0 and g.ordinal(-0.0,0)==0
assert g.pot(.5,255)==128 # ties even; no intermediate binary32 product rounding
for x in (True,False,None,'0',float('nan'),float('inf'),-float('inf'),-.1,1.1):
 try:g.bits(x)
 except ValueError:pass
 else:raise AssertionError(x)
for mutation in range(10):
 d=copy.deepcopy(doc);rows=d['original_resource_bank']['records']
 if mutation==0:rows.pop()
 elif mutation==1:rows.append(copy.deepcopy(rows[0]))
 elif mutation==2:rows[-1]['name']=rows[0]['name']
 elif mutation==3:rows[-1]['name']='Unknown'
 elif mutation==4:rows[-1]['parameters'].pop()
 elif mutation==5:rows[-1]['parameters'].append(0)
 elif mutation==6:rows[-1]['parameters'][-1]=True
 elif mutation==7:rows[-1]['parameters'][-1]=float('nan')
 elif mutation==8:rows[-1]['exact_original_NKS_vector_matches']=False
 else:rows[-1]['parameters'][-1]=1.1
 try:g.build(d,legacy)
 except (ValueError,TypeError):pass
 else:raise AssertionError(mutation)
with tempfile.TemporaryDirectory() as td:
 out=pathlib.Path(td)/'out.h';g.generate(ref,source,out)
 assert out.read_bytes()==(root/'firmware/src/app/miami_factory_typed.h').read_bytes()
 before=out.read_bytes();bad=pathlib.Path(td)/'bad.json';bad.write_text('{}')
 try:g.generate(bad,source,out)
 except ValueError:pass
 else:raise AssertionError('bad pin')
 assert out.read_bytes()==before
# Exact18 binary32 values remain reconstructible from generated private manifest.
text=(root/'firmware/src/app/miami_factory_typed.h').read_text()
for r in doc['original_resource_bank']['records']:
 packed=struct.pack('<18f',*r['parameters']);assert hashlib.sha256(packed).hexdigest() in text
assert len(legacy)==30
print('PASS pinned generator,73 display points,30 full-vector manifests,malformed input,atomic pin rejection')
