"""Reuse exact owned R2 component/paired pixels. Only host navigation is adapted."""
import base64,hashlib,json
from pathlib import Path
R=Path(__file__).parent;OLD=R.parent/'integration-r2-20261114';SRC=R.parent.parent/'dnbox-delivery-recovery/settings-r2/extracted'
def sha(b):return hashlib.sha256(b).hexdigest()
component=(SRC/'index.html').read_bytes();assert sha(component)=='edaf4eb27b6fc2c8199d8d00fbc2492d10600dabe25870ccbb8491f6a23bb2a6'
overview=(SRC/'START-HERE.html').read_text();cases,_=json.JSONDecoder().raw_decode(overview[overview.index('const cases=')+12:]);assets=[]
for c in cases:
 for side,pin in [('actual','before_sha256'),('proposed','after_sha256')]:
  b=(SRC/c[side]).read_bytes();assert sha(b)==c[pin];c[side+'_image']='data:image/png;base64,'+base64.b64encode(b).decode();assets.append({'path':c[side],'bytes':len(b),'sha256':sha(b)})
refs={f:(SRC/f).read_text() for f in ['README.md','source-receipt.json','qa/interaction-results.json','emulator-evidence.json']}
(R/'settings-composition.json').write_text(json.dumps({'cases':cases,'component_b64':base64.b64encode(component).decode(),'component_sha256':sha(component),'references':refs},separators=(',',':'))+'\n')
section=(OLD/'settings-section.html').read_text().replace('The accepted R2 component','The separately owned R2 component').replace('The whole proposal uses Barlow and persistent power context.','R6 uses Barlow and consistent top power. Component/header integration remains pending with the Settings owner.');(R/'settings-section.html').write_text(section)
adapter=(OLD/'settings-host.js').read_text();a=adapter.index("    $('screenChoice').value");b=adapter.index("  };",a);adapter=adapter[:a]+"    document.querySelector('[data-tab=\\\"model\\\"]').click();\n    proposal.reset('load');\n    document.getElementById('lcd').scrollIntoView({block:'nearest'});\n"+adapter[b:];(R/'settings-host.js').write_text(adapter)
(R/'evidence/SETTINGS-R2-INTAKE.json').write_text(json.dumps({'component_owner':'6af84d26-84d0-8191-9e55-0448748ca0dc','zip_sha256':'3ba167e0b01c21e08a14eb84d87d2c3b074d46b16b8960319e81c66aabaabc9d','library_version':1,'library_file':'file_00000000519481f590b8e601e2c82862','component_sha256':sha(component),'component_bytes':len(component),'paired_assets':assets,'component_edit':'NONE; exact original bytes decoded into isolated iframe; separate host bridge only','evidence_reuse':'Existing Settings105 and prior composition54 remain separately scoped; changed R6 host navigation checks only','versions':'R2 actual-before historical requested25.1/registry0.0.25; component94197 proposal; not current3c41','pending':'Common header/power grammar and whole held-note continuity remain component-owner interfaces; no redesign or runtime approval'},indent=2)+'\n')
print(json.dumps({'component_sha256':sha(component),'paired_assets':len(assets)}))
