"""Assemble the portable R7 design walkthrough."""
import base64,hashlib,json
from pathlib import Path
R=Path(__file__).parent
def sha(d):return hashlib.sha256(d).hexdigest()
def build():
 p=json.loads((R/'parameter-control-map.json').read_text());d=json.loads((R/'drum-function-map.json').read_text());pairs=json.loads((R/'comparisons.json').read_text());refs=[]
 for x in pairs:
  raw=(R/'reference'/x['before']).read_bytes();x['before_image']='data:image/png;base64,'+base64.b64encode(raw).decode();x['before_sha256']=sha(raw);refs.append({'file':x['before'],'sha256':sha(raw),'evidence':'Actual pre-existing3c41 Elite emulator LCD; not proposal execution'})
 data={'source_commit':'3c41ef4bedf9701f549ce2752b73306e2fab1eb8','source_tree':'fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d','engines':p['engines'],'drums':d['drum_voice_map'],'trackMaps':p['drums'],'miamiUI':json.loads((R/'miami-ui-drawings.json').read_text()),'chicagoWave':json.loads((R/'chicago-wave-drawing.json').read_text()),'typedDrums':json.loads((R/'typed-drum-parameters.json').read_text())['kits'],'rates':['1/16','1/16T','1/32','1/8T','1/8','1/4'],'pairs':pairs,'journeys':json.loads((R/'evidence/whole-journey-audit.json').read_text())['groups'],'guides':json.loads((R/'journey-walkthroughs.json').read_text()),'counts':json.loads((R/'source-access-counts.json').read_text())['scenarios']}
 data['settings']=json.loads((R/'settings-composition.json').read_text())
 data['nativeClips']={name:'data:image/png;base64,'+base64.b64encode((R/'reference'/file).read_bytes()).decode() for name,file in [('wait','elite-clip-wait-v1-01-no-host-waiting.png'),('scale','elite-clip-wait-v1-02-clip-scale-page.png'),('miami','elite-default-A-v1-01-default-A-sound-native.png')]}
 if (R/'proposal-images.json').exists():data['proposalImages']=json.loads((R/'proposal-images.json').read_text())
 fonts='\n'.join('@font-face{font-family:Barlow;font-weight:'+str(w)+';src:url(data:font/ttf;base64,'+base64.b64encode((R/'fonts'/f).read_bytes()).decode()+')}' for w,f in [(500,'BarlowSemiCondensed-Medium.ttf'),(600,'BarlowSemiCondensed-SemiBold.ttf'),(700,'BarlowSemiCondensed-Bold.ttf')])
 js=(R/'proposal.js').read_text();out=(R/'workbench.template.html').read_text().replace('/*FONTS*/',fonts).replace('/*DATA*/',json.dumps(data).replace('</','<\\/')).replace('/*RENDERER*/',js).replace('/*GENERATOR-DELTA*/',(R/'physical-role-router.js').read_text()+'\n'+(R/'generator-math.js').read_text()+'\n'+(R/'generator-proposal.js').read_text()).replace('<!--SETTINGS-->',(R/'settings-section.html').read_text()).replace('/*SETTINGS-HOST*/',(R/'settings-host.js').read_text())
 (R/'workbench.html').write_text(out);receipt={'html_sha256':sha(out.encode()),'renderer_sha256':sha(js.encode()),'source':data['source_commit'],'reviewed_predecessor':'8eb1a6888562fcdb6c54fb7b7845f45a0de48e86','current_references':refs,'evidence':'Proposal model only; exact successor independent review pending'};(R/'model-inputs.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt))
if __name__=='__main__':build()
