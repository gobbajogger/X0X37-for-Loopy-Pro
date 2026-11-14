"""Build portable R4 proposal only, never firmware."""
import base64,hashlib,json
from pathlib import Path
R=Path(__file__).parent
def sha(d):return hashlib.sha256(d).hexdigest()
def build():
 p=json.loads((R/'parameter-control-map.json').read_text());d=json.loads((R/'drum-function-map.json').read_text());pairs=json.loads((R/'comparisons.json').read_text());refs=[]
 for x in pairs:
  raw=(R/'reference'/x['before']).read_bytes();x['before_image']='data:image/png;base64,'+base64.b64encode(raw).decode();x['before_sha256']=sha(raw);refs.append({'file':x['before'],'sha256':sha(raw),'evidence':'Actual pre-existing3c41 Elite emulator LCD; not proposal execution'})
 data={'source_commit':'3c41ef4bedf9701f549ce2752b73306e2fab1eb8','source_tree':'fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d','engines':p['engines'],'drums':d['drum_voice_map'],'trackMaps':p['drums'],'rates':['1/16','1/16T','1/32','1/8T','1/8','1/4'],'pairs':pairs,'counts':json.loads((R/'source-access-counts.json').read_text())['scenarios']}
 if (R/'proposal-images.json').exists():data['proposalImages']=json.loads((R/'proposal-images.json').read_text())
 fonts='\n'.join('@font-face{font-family:Barlow;font-weight:'+str(w)+';src:url(data:font/ttf;base64,'+base64.b64encode((R/'fonts'/f).read_bytes()).decode()+')}' for w,f in [(500,'BarlowSemiCondensed-Medium.ttf'),(600,'BarlowSemiCondensed-SemiBold.ttf')])
 js=(R/'proposal.js').read_text();out=(R/'workbench.template.html').read_text().replace('/*FONTS*/',fonts).replace('/*DATA*/',json.dumps(data).replace('</','<\\/')).replace('/*RENDERER*/',js)
 (R/'workbench.html').write_text(out);receipt={'html_sha256':sha(out.encode()),'renderer_sha256':sha(js.encode()),'source':data['source_commit'],'reviewed_predecessor':'96b4f43498a089b326e463097b2e9641458e4589','current_references':refs,'evidence':'Proposal model only; exact successor independent review pending'};(R/'model-inputs.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':build()
