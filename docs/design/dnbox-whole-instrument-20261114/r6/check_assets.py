"""Verify final native/body pixels, embedded pair inputs and standard PDF delivery."""
import base64,gzip,hashlib,io,json
from pathlib import Path
import fitz
from PIL import Image
R=Path(__file__).parent
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
 checks=[];pages=[]
 def ck(n,v,d=None):checks.append({'name':n,'pass':bool(v),'detail':d})
 pairs=json.loads((R/'comparisons.json').read_text());images=json.loads((R/'proposal-images.json').read_text());manifest=json.loads((R/'CURATED-NATIVE-MANIFEST.json').read_text());renderer=sha((R/'proposal.js').read_bytes());html=(R/'workbench.html').read_bytes()
 ck('Curated native inventory binds final exact renderer',manifest['renderer_sha256']==renderer)
 for p in pairs:
  before=R/'reference'/p['before'];after=R/'native'/p['after'];a=Image.open(after);b=Image.open(before)
  ck('Native pair dimensions and embedded after bytes: '+p['id'],a.size==b.size==(240,240) and base64.b64decode(images[p['id']].split(',',1)[1])==after.read_bytes())
  ck('Final HTML embeds exact before and after: '+p['id'],base64.b64encode(before.read_bytes()) in html and base64.b64encode(after.read_bytes()) in html)
 for f in manifest['frames']:
  ck('Final native file hash matches frame recipe: '+f['id'],sha((R/f['file']).read_bytes())==f['sha256'])
  bad=[x for x in f['bounds'] if x['x']<0 or x['x']+x['width']>240.1 or x['y']<0 or x['y']+x['height']>240]
  ck('Native text stays within240x240: '+f['id'],not bad,bad)
 for fixture,key in [('clip','clip'),('miami-native','miami-native')]:
  p=next(x for x in pairs if x['id']==key);a=Image.open(R/'native'/p['after']).convert('RGB');b=Image.open(R/'reference'/p['before']).convert('RGB');ck('Original body/lower pixels retained exactly: '+fixture,a.crop((0,20,240,240)).tobytes()==b.crop((0,20,240,240)).tobytes())
 for name in ['review-correction-checks.json','musical-journey-checks.json','delivery-route-checks.json']:
  v=json.loads((R/name).read_text());ck('Affected checks pass with exact input or declared unchanged-model reuse: '+name,v['passed']==v['total'] and (v['renderer_sha256']==renderer or name=='musical-journey-checks.json' and json.loads((R/'evidence/MODEL-PROOF-REUSE.json').read_text())['prior_renderer_sha256']==v['renderer_sha256']))
  if 'complete_verification_inventory' in v:
   inv=v['complete_verification_inventory'];packed=(R/inv['path']).read_bytes();raw=gzip.decompress(packed);ck('Lossless verification inventory hashes: '+name,sha(packed)==inv['sha256'] and sha(raw)==inv['uncompressed_sha256'])
 doc=fitz.open(R/'DnBOX-R6-musical-workflows.pdf');ck('Standard unencrypted15pagePDF parses',len(doc)==len(pairs)+2 and not doc.is_encrypted)
 out=R.parent/'continuation-evidence/r6-pdf-inspection';out.mkdir(exist_ok=True)
 for n,page in enumerate(doc):
  pix=page.get_pixmap(matrix=fitz.Matrix(1.0,1.0),alpha=False);path=out/f'page-{n+1:02}.png';pix.save(path);bad=[]
  for block in page.get_text('dict')['blocks']:
   if block.get('type')==0:
    for line in block['lines']:
     for span in line['spans']:
      box=fitz.Rect(span['bbox'])
      if not page.rect.contains(box):bad.append(span['text'])
  ck('PDF page renders and text stays inside page: '+str(n+1),not bad and pix.width>500,bad)
  if n<len(pairs):
   p=pairs[n];expected=[Image.open(R/'reference'/p['before']).convert('RGB').tobytes(),Image.open(R/'native'/p['after']).convert('RGB').tobytes()];actual=[Image.open(io.BytesIO(doc.extract_image(x[0])['image'])).convert('RGB') for x in page.get_images()];ck('PDF embeds both original native pixel arrays: '+p['id'],len(actual)==2 and sorted(sha(im.tobytes()) for im in actual)==sorted(sha(x) for x in expected))
  pages.append({'page':n+1,'render_bytes':path.stat().st_size,'render_sha256':sha(path.read_bytes()),'local_inspection_file':str(path),'extracted_text_characters':len(page.get_text())})
 receipt={'classification':'Producer artifact/pixel/PDF verification, not independent musical or physical acceptance','renderer_sha256':renderer,'html_sha256':sha(html),'pdf_sha256':sha((R/'DnBOX-R6-musical-workflows.pdf').read_bytes()),'checks':checks,'passed':sum(x['pass'] for x in checks),'total':len(checks),'pdf_page_inspection':pages,'limits':['No actual iOS device run; standard PDF parsed and all pages rasterized','Native raster and browser colors are not physical display evidence','This inventory does not complete the whole design']};(R/'asset-checks.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'passed':receipt['passed'],'total':receipt['total'],'failures':[x for x in checks if not x['pass']]}))
main()
