"""R7 native/provenance/PDF/privacy checks; no firmware or device execution."""
from pathlib import Path
import base64,hashlib,io,json,re
from PIL import Image
import fitz
R=Path(__file__).parent
def sha(b):return hashlib.sha256(b).hexdigest()
checks=[]
def ck(name,ok,detail=None):checks.append({'name':name,'pass':bool(ok),'detail':detail})
pairs=json.loads((R/'comparisons.json').read_text());html=(R/'workbench.html').read_text();model=json.loads((R/'model-inputs.json').read_text());native=json.loads((R/'CURATED-NATIVE-MANIFEST.json').read_text());actual={p['file']:p for p in json.loads((R/'technical-evidence/ACTUAL-REFERENCE-HASHES.json').read_text())['files']}
ck('Final assembled HTML matches declared model input',sha(html.encode())==model['html_sha256'])
for n in ['proposal.js','physical-role-router.js','generator-math.js','generator-proposal.js','settings-host.js']:
 ck('Final HTML contains exact source: '+n,(R/n).read_text() in html)
for p in pairs:
 before=(R/'reference'/p['before']).read_bytes();after=(R/'native'/p['after']).read_bytes()
 ck('Actual-before matches pinned source receipt: '+p['id'],sha(before)==actual['reference/'+p['before']]['sha256'] and len(before)==actual['reference/'+p['before']]['bytes'])
 ck('Native pair embedded exactly in final HTML: '+p['id'],base64.b64encode(before).decode() in html and base64.b64encode(after).decode() in html)
 for kind,raw in [('before',before),('after',after)]:ck('Native240 geometry '+p['id']+' '+kind,Image.open(io.BytesIO(raw)).size==(240,240))
for p in sorted((R/'reference').glob('*.png')):ck('Reference native240: '+p.name,Image.open(p).size==(240,240))
ck('Only15 curated PNGs in owner native directory',{p.name for p in (R/'native').glob('*.png')}=={p['after'] for p in pairs})
for n in ['GEN-AFFECTED-CHECKS.json','PHYSICAL-DOM-CHECKS.json']:
 d=json.loads((R/n).read_text());ck('Focused checks pass: '+n,d['passed']==d['total']);ck('Focused input identity retained: '+n,d['html_sha256']==model['html_sha256'])
d=json.loads((R/'GENERATOR-MATH-CHECKS.json').read_text());ck('Generator C comparison132 exact model input',d['passed']==132==d['total'] and d['model_sha256']==sha((R/'generator-math.js').read_bytes()) and d['reference_sha256']==sha((R/'technical-evidence/generator-reference-fixtures.json').read_bytes()))
d=json.loads((R/'DELIVERY-DELTA-CHECKS.json').read_text());ck('Final seven guides/Settings host53 checks exact HTML',d['passed']==53==d['total'] and d['html_sha256']==model['html_sha256'])
for f in native['frames']:
 ck('Curated frame hash: '+f['id'],sha((R/f['file']).read_bytes())==f['sha256'])
 ck('Curated label/value text stays in native bounds: '+f['id'],all(x['x']>=0 and x['x']+x['width']<=240.1 and x['y']>=0 and x['y']+x['height']<=240 for x in f['bounds']))
doc=fitz.open(R/'DnBOX-R7-owner-comparisons.pdf');ck('PDF has15pair pages plus2 scope pages',len(doc)==17)
texts=[]
for i,page in enumerate(doc):
 text=page.get_text();texts.append(text);bad=[]
 for b in page.get_text('dict')['blocks']:
  for line in b.get('lines',[]):
   for sp in line['spans']:
    q=sp['bbox']
    if q[0]<0 or q[1]<0 or q[2]>page.rect.width+.1 or q[3]>page.rect.height+.1:bad.append(sp['text'])
 ck('PDF page text fits '+str(i+1),not bad,bad)
 images=page.get_images(full=True)
 if i<len(pairs):
  expected=[Image.open(R/'reference'/pairs[i]['before']).convert('RGB').tobytes(),Image.open(R/'native'/pairs[i]['after']).convert('RGB').tobytes()];actual=[fitz.Pixmap(doc,im[0]).samples for im in images]
  ck('PDF exact240 RGB pair '+str(i+1),len(images)==2 and sorted(actual)==sorted(expected) and all(im[2:4]==(240,240) for im in images))
pdf='\n'.join(texts)
patterns=[r'/(?:workspace|root)/',r'link_[0-9a-f]+',r'\b[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\b',r'https://chatgpt\.com/c/',r'Parent reports',r'HTTP401']
ck('Owner PDF contains no private paths/routing/IDs',not any(re.search(x,pdf,re.I) for x in patterns))
for name in ['workbench.template.html','settings-host.js','settings-section.html','README.md','WHOLE-INSTRUMENT-DESIGN.md','DISPLACEMENT-AND-ACCESS.json','WHOLE-FUNCTION-CROSSWALK.json','evidence/whole-journey-audit.json','journey-walkthroughs.json','settings-composition.json']:
 ck('Owner-facing privacy scan: '+name,not any(re.search(x,(R/name).read_text(),re.I) for x in patterns))
for n in ['README.md','WHOLE-INSTRUMENT-DESIGN.md','evidence/whole-journey-audit.json']:ck('Owner link resolves: '+n,(R/n).is_file())
receipt={'classification':'Producer static native/PDF/input/privacy verification; not independent acceptance or physical/iOS proof','html_sha256':sha(html.encode()),'pdf_sha256':sha((R/'DnBOX-R7-owner-comparisons.pdf').read_bytes()),'passed':sum(c['pass'] for c in checks),'total':len(checks),'checks':checks,'limits':['No actual iOS device opening','PNG/PDF parser/embedded pixels checked; display density/lighting/physical recognition not measured','Prior refused file-URI navigation not retried; this script is static only']}
(R/'ASSET-CHECKS.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'passed':receipt['passed'],'total':receipt['total'],'failures':[c for c in checks if not c['pass']]}));assert receipt['passed']==receipt['total']
