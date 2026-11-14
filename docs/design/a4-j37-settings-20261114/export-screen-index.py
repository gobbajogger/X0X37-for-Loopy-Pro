"""Create a light native-screen index using the actual proposal renderer."""
from pathlib import Path
from urllib.parse import quote
ROOT=Path(__file__).resolve().parent/'qa'
SCENES=['Category focus','Last MIDI row','Bluetooth status','USB AUDIO','Reset / cancel','Clip action mask','Long peer name','Held local key']
html='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>A4/J37 native Settings screen index</title><style>body{background:#101719;color:#e3f2e7;font:14px system-ui;padding:20px}main{display:grid;grid-template-columns:repeat(auto-fit,256px);gap:16px}figure{margin:0}iframe{width:240px;height:240px;border:0}a{color:#80d9c4}figcaption{padding:8px 0}</style><h1>A4 / J37 · proposed native screens</h1><p>Design simulation; these240×240 frames use the same renderer as the interaction walkthrough. The complete private bundle contains deterministic PNG exports, including edge states. These frames are not firmware renders.</p><p><a href="../index.html">Open interaction walkthrough</a></p><main>'''
for name in SCENES:
    html+=f'<figure><iframe title="{name}" src="../index.html?screen={quote(name)}" loading="eager"></iframe><figcaption>{name}</figcaption></figure>'
html+='</main></html>'
(ROOT/'reviewed-screens.html').write_text(html+'\n')
(ROOT/'reviewed-screens.svg').unlink(missing_ok=True)
print(ROOT/'reviewed-screens.html')
