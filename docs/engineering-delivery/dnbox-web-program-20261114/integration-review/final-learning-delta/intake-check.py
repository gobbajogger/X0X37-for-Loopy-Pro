# Hash-only source/host intake. Private component payload is not retained in evidence.
from pathlib import Path
import hashlib,json,datetime,difflib,urllib.request
out=Path(__file__).resolve().parent;program=out.parents[1];site=program/'site';prior=program/'history/ed518381/site';private=Path('/workspace/dnbox-private-integration/host')
sha=lambda b:hashlib.sha256(b).hexdigest()
def manifest_info(root,include_names):
 raw=(root/'MANIFEST.sha256').read_bytes();checks=[]
 for line in raw.decode().splitlines():
  expected,name=line.split(None,1);actual=sha((root/name).read_bytes());checks.append({'file':name,'expected':expected,'actual':actual,'match':actual==expected})
 result={'manifest_sha256':sha(raw),'entries':len(checks),'own_files':sum(p.is_file() for p in root.rglob('*')),'all_entries_match':all(x['match'] for x in checks)}
 if include_names:result['entry_checks']=checks
 return result
public=manifest_info(site,True);private_info=manifest_info(private,False)
expected='44188b31a644cde2b7992547faf2f3c6a51b0f729ba7161cf88b168362243496';assert public['manifest_sha256']==expected
assert private_info['manifest_sha256']=='99e51da2442eac9e081f8698018985159d157cde1d5bdb0dcaf029638ad1c8d6'
old={line.split(None,1)[1]:line.split(None,1)[0] for line in (prior/'MANIFEST.sha256').read_text().splitlines()};current={x['file']:x['actual'] for x in public['entry_checks']}
source=[]
for name in ['app.js','journeys.js','provenance/integration-20261114.md']:
 previous=(prior/name).read_bytes();now=(site/name).read_bytes();assert sha(previous)==old[name]
 source.append({'file':name,'previous_sha256':sha(previous),'current_sha256':sha(now),'private_byte_identical':now==(private/name).read_bytes(),'changed_line_ranges':[[i1+1,i2,j1+1,j2] for op,i1,i2,j1,j2 in difflib.SequenceMatcher(a=previous.decode().splitlines(),b=now.decode().splitlines()).get_opcodes() if op!='equal'],'append_only':now.startswith(previous) if name.startswith('provenance/') else None,'served_public_sha256':sha(urllib.request.urlopen('http://127.0.0.1:4323/'+name).read()),'served_private_sha256':sha(urllib.request.urlopen('http://127.0.0.1:4324/'+name).read())})
packet={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'public':public,'prior_manifest_sha256':sha((prior/'MANIFEST.sha256').read_bytes()),'private_aggregate_only':private_info,'changed_public_entries':[p for p in old if old[p]!=current.get(p)],'unchanged_public_entries':sum(old[p]==current.get(p) for p in old),'source_checks':source,'historical_report_sha256':sha((out.parent/'REPORT.md').read_bytes()),'historical_evidence_manifest_sha256':sha((out.parent/'EVIDENCE.sha256').read_bytes()),'prior_successor_report_sha256':sha((out.parent/'successor-4998572/REPORT.md').read_bytes()),'prior_successor_evidence_manifest_sha256':sha((out.parent/'successor-4998572/EVIDENCE.sha256').read_bytes()),'private_payload_copied':False,'scope':'Three website source files and aggregate private manifest only; no component payload, baseline suite, device, deployment or launcher action'}
assert packet['prior_manifest_sha256']=='ed518381e86d16fa2a104d156912b5da11d4c180b9f02764e45d4c89554ee00a'
assert len(packet['changed_public_entries'])==3 and all(x['private_byte_identical'] and x['current_sha256']==x['served_public_sha256']==x['served_private_sha256'] for x in source)
(out/'intake-results.json').write_text(json.dumps(packet,indent=2));print(json.dumps({k:v for k,v in packet.items() if k!='public'},indent=2));print('Public all69entrycheck',public['all_entries_match'])
