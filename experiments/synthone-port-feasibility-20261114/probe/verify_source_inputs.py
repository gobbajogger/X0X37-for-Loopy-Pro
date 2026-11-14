"""Verify external public source and packaged dependencies before generation."""
from pathlib import Path
import os,json,hashlib
P=Path(__file__).resolve().parent
source=Path(os.environ['SYNTHONE_SOURCE']).expanduser().resolve()
if not (source/'DSP').is_dir() or not (source/'Presets').is_dir():
 raise ValueError('SYNTHONE_SOURCE must be the inner AudioKitSynthOne directory containing DSP and Presets')
roots={'AudioKit/AudioKitSynthOne':source.parent, 'AudioKit/AudioKit':P.parent/'audiokit-4.9.2'}
items=json.loads((P.parent/'public-source-manifest.json').read_text())
count=0
for item in items:
 base=roots[item['repository']]
 path=(base/item['path']).resolve()
 if not path.is_relative_to(base.resolve()): raise ValueError('source path escapes root')
 raw=path.read_bytes()
 if len(raw)!=item['size'] or hashlib.sha256(raw).hexdigest()!=item['sha256']:
  raise ValueError('public source identity mismatch: '+item['path'])
 count+=1
print('PUBLIC_SOURCE_IDENTITIES_PASS',count)
