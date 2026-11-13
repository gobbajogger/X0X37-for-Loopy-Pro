#!/usr/bin/env python3
"""Check accepted initial patches against the current combined firmware base."""
import hashlib,json,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
E=Path(__file__).resolve().parent
BASE='1165396199210889fa460d26cbdd0b66183267e1'
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT)
def digest(data):return hashlib.sha256(data).hexdigest()
def command(args,**kw):
 p=subprocess.run(args,cwd=ROOT,capture_output=True,**kw)
 assert p.returncode==0,(args,p.returncode,p.stderr.decode(errors='replace'))
 return p.stdout
result={'base':BASE,'checked_head':git('rev-parse','HEAD').decode().strip(),'scope':'Initial accepted host/browser patches only; prerequisites remain pending.'}
assert not git('diff',BASE,'--','firmware')
result['firmware_tree']=git('rev-parse',BASE+':firmware').decode().strip()
result['firmware_unchanged']=True
accepted=json.loads((ROOT.parent/'host_setup_stage4/evidence/host-setup-stage4/result.json').read_text())['accepted_source_sha256']
assert all(digest((ROOT/p).read_bytes())==h for p,h in accepted.items())
result['host_accepted_source_hashes']=accepted
# Retain canonical generated headers only as preprocessor inputs. This does not generate or validate a package.
rows=json.loads((ROOT.parent/'build_speed_consumer_audit/evidence/cache-consumer-audit/saved-header-equality.json').read_text())['rows']
with tempfile.TemporaryDirectory(prefix='x0x-combination-inputs-') as temp:
 gen=Path(temp)
 for row in rows:
  raw=(ROOT.parent/'build_speed_consumer_audit/build/cache-consumer-baseline'/row['name']).read_bytes()
  assert digest(raw)==row['baseline_sha256']
  (gen/row['name']).write_bytes(raw)
 argv=['cc','-E','-P','-x','c','-std=c99','-DX0X_HOST','-Ifirmware/src','-Ifirmware/src/dsp','-I'+str(gen),'-Ihost','-']
 before=command(argv,input=git('show',BASE+':host/x0x_host.c'))
 after=command(argv,input=(ROOT/'host/x0x_host.c').read_bytes())
 assert before==after,'Browser removal changed native preprocessed source'
 result['native_preprocessor']={'result':'PASS','sha256':digest(after),'compiler':command(['cc','--version']).decode()}
# Recheck current combined documentation/site inputs, preserving all newer text.
sys.path.insert(0,str(ROOT/'tools'))
import make_pages
with tempfile.TemporaryDirectory(prefix='x0x-combination-site-') as temp:
 scratch=Path(temp); sites=[]
 for product in ('FM-1_9001000','SMK-37 Pro_9001000','SMK-37 Elite_9001000','MKE-P37_9001000'):
  raw=bytearray(b'\0'*(20*0x30))
  for i in range(20):raw[i*0x30+0x2f]=(ord(product[i])+i+1)&255 if i<len(product) else 0x7d
  raw+=b'FELUCCA-LOADER-1'; pkg=scratch/'fixture.fwsc'; pkg.write_bytes(raw)
  out=scratch/'site'; make_pages.main(pkg,'combination-check',out)
  page=(out/'new/index.html').read_text()
  assert 'The browser version can load your own breaks.' not in page
  assert 'href="../breaks/"' in page
  assert 'requestMIDIAccess' in (out/'breaks/index.html').read_text()
  assert 'installKeepingSlots' in (out/'install/index.html').read_text()
  assert not (out/'emu').exists()
  sites.append(product)
 result['site_identities']=sites
command(['git','diff','--check',BASE])
result['checks']='PASS firmware/source identity, native preprocessing equality, four site identities and diff whitespace.'
result['limits']=['No host/target/controller build or install. Marker packages check site generation only.','Runner class cleanup and stable validator cleanup remain pending checked prerequisites.','Host fixtures and production probes reuse accepted evidence until a consumer path changes.']
(E/'initial-checks.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
