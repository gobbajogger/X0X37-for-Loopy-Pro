"""Recognition templates only. Never draws or edits a returned firmware display.
Native Rust panel truncates RGB565 expansion; old qualification canvas rounded.
Invert the unambiguous RGB565 levels before applying the original Rust conversion.
"""
from pathlib import Path
from PIL import Image
import json,base64,hashlib
ROOT=Path(__file__).resolve().parent
initial=(ROOT/'evidence/pro-initial.rgba').read_bytes()
source=Path('/workspace/X0X37-for-Loopy-Pro/docs/dnbox-web-program-20261114/site/assets/firmware/elite-909-lcd.png')
im=Image.open(source).convert('RGBA')
def native(pixel):
 return tuple(round(v*n/255)*255//n for v,n in zip(pixel[:3],(31,63,31)))+(255,)
dirty=bytes(v for y in range(240) for x in range(240) for v in native(im.getpixel((x*2,y*2))))
on=bytearray(initial)
for c in range(16):
 x=27+c*13+(c>>2)
 for y in range(21,30):
  on[(y*240+x)*4:(y*240+x+12)*4]=dirty[(y*240+27)*4:(y*240+39)*4]
b64=lambda x:base64.b64encode(x).decode()
data={'source':'79280e763341d03ba43266d3e25dad66f12de860','scope':'Observed Pro initial plus retained Elite four-kick signatures; recognition only; not generated display data','signatures':[b64(initial),b64(dirty)],'off':b64(initial),'on':b64(on)}
data['rowLanes']=[]
for path in sorted((ROOT/'evidence/row-family').glob('*.rgba')):
 pixels=path.read_bytes();assert len(pixels)==240*240*4
 lane=bytes(v for y in range(20,158) for v in pixels[(y*240)*4:(y*240+27)*4])
 data['rowLanes'].append(b64(lane))
assert len(data['rowLanes'])==11
data['saveMessages']=[]
for text,name,sha in [
 ('SAVES WHEN STOPPED','save-while-running-350ms-rgba.bin','61c239db57fe4f741d14049c267fea63b30278d088909a5b8eb0b43547d0606c'),
 ('SAVED','save-after-stop-450ms-rgba.bin','91681e2fbc98c226ff445a334e9350c50605e8f81a7204ed0420c46adc50defb')]:
 pixels=(ROOT/'review/workflow'/name).read_bytes();assert hashlib.sha256(pixels).hexdigest()==sha
 data['saveMessages'].append({'text':text,'header':b64(pixels[:240*20*4]),'native_capture_sha256':sha})
(ROOT/'site/firmware-lesson/references.json').write_text(json.dumps(data,separators=(',',':'))+'\n')
(ROOT/'evidence/reference-provenance.json').write_text(json.dumps({'native_initial_sha256':hashlib.sha256(initial).hexdigest(),'retained_reference_png_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'conversion':'Exact inverse RGB565, then floor(channel*255/31 or63), matching panel_smk.rs; recognition templates only','source_regions':'ui.c col_x/DROW; BD y21..29, x27..38; no actual screen written','reference_json_sha256':hashlib.sha256((ROOT/'site/firmware-lesson/references.json').read_bytes()).hexdigest()},indent=2)+'\n')
