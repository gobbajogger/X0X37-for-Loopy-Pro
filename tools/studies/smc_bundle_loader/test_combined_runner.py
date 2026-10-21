# SPDX-License-Identifier: GPL-3.0-only
"""Safety checks must remain active with python3 -O; no target build here."""
import importlib.util
from pathlib import Path
import tempfile

spec=importlib.util.spec_from_file_location('combined',Path(__file__).with_name('build_combined.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp);(p/'tools').mkdir();(p/'build').mkdir()
    called=[]
    def fake(*args,**kwargs): called.append(args)
    m.run=fake
    recipe=p/'tools/build.py'
    for text in ('    pkg = fm1pkg_make.ufw()',
                 '    print("NON-INSTALLABLE")\n    pkg = fm1pkg_make.ufw()'):
        recipe.write_text(text)
        try:m.build(p,p/'log')
        except RuntimeError:pass
        else:raise RuntimeError('unsafe recipe accepted')
        if called:raise RuntimeError('unsafe build executed')
    recipe.write_text('    print("NON-INSTALLABLE")\n    return 0\n    pkg = fm1pkg_make.ufw()')
    m.build(p,p/'log')
    if len(called)!=1:raise RuntimeError('valid guarded recipe not run')
    (p/'build/unexpected.fwsc').write_bytes(b'not a firmware')
    try:m.build(p,p/'log')
    except RuntimeError:pass
    else:raise RuntimeError('unexpected package not detected')
print('combined runner: missing return/label refused before build, unexpected package detected PASS')
