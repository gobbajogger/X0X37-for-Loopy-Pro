"""Lossless separate verification inventory plus compact screen-state excerpts."""
import copy,gzip,hashlib,json
from pathlib import Path
def digest(b):return hashlib.sha256(b).hexdigest()
def compact_state(state):
 d=copy.deepcopy(state);allp=d.pop('patternData',None)
 if allp is None:return d
 selected=set(d.get('currentPatterns',[]))|{x for x in d.get('pendingPatterns',[]) if x>=0}
 if d.get('copyRequest'):selected|=set(d['copyRequest']['source'])|{d['copyRequest']['dest']}
 selected|={i for i,p in enumerate(allp) if any(any(a for a in k['steps']) for k in p['drums']) or any(any(n['gate']!='REST' for n in b['notes']) for b in p['bass']) or p['brk']['steps']}
 d['patternData_excerpt']={}
 for i in sorted(selected):
  p=allp[i]
  for b in p['bass']:
   notes=b.pop('notes');b['notes_nonrest']={str(j+1):n for j,n in enumerate(notes) if n['gate']!='REST'};b['total_step_cells']=len(notes)
  d['patternData_excerpt'][str(i)]=p
 d['pattern_count']=len(allp);d['state_sha256_canonical_json']=digest(json.dumps(state,sort_keys=True,separators=(',',':')).encode());return d

def save_receipt(root,name,receipt):
 raw=(json.dumps(receipt,indent=2)+'\n').encode();packed=gzip.compress(raw,mtime=0);v=root/'verification';v.mkdir(exist_ok=True);stem=name.removesuffix('.json')+'-full.json.gz';(v/stem).write_bytes(packed)
 out=copy.deepcopy(receipt)
 for f in out.get('frames',[]):
  if 'state' in f:f['state']=compact_state(f['state'])
 out['complete_verification_inventory']={'path':'verification/'+stem,'bytes':len(packed),'sha256':digest(packed),'uncompressed_bytes':len(raw),'uncompressed_sha256':digest(raw),'class':'Lossless producer proposal/model evidence only; no firmware/device acceptance'}
 (root/name).write_text(json.dumps(out,indent=2)+'\n');return out
