import ast,json,hashlib
from pathlib import Path
O=Path(__file__).resolve().parent;src=O.parents[1]/'sources/scene-proof/add_wordmark.py';tree=ast.parse(src.read_text());G=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='G' for t in n.targets))
widths=[.71,.74,.70,.72,.76];gap=.045;total=sum(widths)+gap*4
for mode in ['mono','accent']:
 paths=[];x=0
 for i,((letter,polys),w) in enumerate(zip(G.items(),widths)):
  d=' '.join('M'+' L'.join(f'{100*(px+x):.3f},{100*(1-py):.3f}' for px,py in poly)+' Z' for poly in polys)
  fill='currentColor' if mode=='mono' or i<2 else '#8fbf79';paths.append(f'<path data-letter="{letter}" fill="{fill}" fill-rule="evenodd" d="{d}"/>');x+=w+gap
 text=f'<svg xmlns="http://www.w3.org/2000/svg" color="#f0f2e9" viewBox="0 0 {total*100:.1f} 100" role="img" aria-label="DnBOX">\n<title>DnBOX</title>\n'+''.join(paths)+'\n</svg>\n';(O/f'DnBOX-custom-{mode}.svg').write_text(text)
(O/'README.md').write_text('''# DnBOX source wordmark

Original custom vector artwork, exported from the same SLAB Dn / CUT BOX contours currently used in the Blender scene. One shared B; uppercase D/B/O/X, lowercase n. No external logo/font outlines or audio/captures are included.

This is the owner-selected construction used by the current work, not a declaration that the unfinished motion or complete site has been accepted. Root owns publication/preview gates.

`mono` inherits CSS currentColor; `accent` accents shared B + OX with Night lime. Do not distort individual letters or split the shared B into two glyphs. Transparent vector canvas; use original viewBox/aspect ratio. Export script and source-path receipt make exact geometry reproducible.

No audio license or third-party trademark clearance is implied.
''')
(O/'asset-receipt.json').write_text(json.dumps({'source':str(src),'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'construction':'custom SLAB Dn + CUT BOX, shared B','svg':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in O.glob('*.svg')}},indent=2))
print(O)
