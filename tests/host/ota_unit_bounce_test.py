#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""No-device native OTA transport regression: foreign SysEx bounce must taint link.

Run from repository root:
    python3 -B tests/host/ota_unit_bounce_test.py tools/fm1_install.py

The current MidoLink class is loaded directly from the installer AST; synthetic
MIDI and mock per-device unit tokens stand in for hardware. An authenticated
physical-ID provider is NOT implemented in the production MidoBackend.
"""
import ast, copy, pathlib, queue, sys, time
from types import ModuleType, SimpleNamespace

class FakeIn:
    def __init__(self, callback): self.callback=callback; self.closed=False
    def inject(self, *data): self.callback(SimpleNamespace(type='sysex',data=list(data)))
    def close(self): self.closed=True
class FakeOut:
    def __init__(self): self.sent=[]; self.closed=False
    def send(self,msg): self.sent.append(bytes(msg.data))
    def close(self): self.closed=True
class FakeMido(ModuleType):
    def __init__(self): super().__init__('mido'); self.inputs=[]; self.outputs=[]
    def open_input(self, name,callback):
        x=FakeIn(callback);self.inputs.append(x);return x
    def open_output(self,name):
        x=FakeOut();self.outputs.append(x);return x
    def Message(self,*a,**kw): return SimpleNamespace(data=kw['data'])
class Backend:
    def __init__(self,token='unit-A',name='SMK-37 Pro'):
        self.token,self.name=token,name
    def physical_id(self,name): return self.token if name==self.name else None
    def output_physical_id(self,name): return self.token if name==self.name else None
    def input_names(self): return [self.name]

def load(path):
    source=pathlib.Path(path).read_text()
    tree=ast.parse(source)
    cls=next(n for n in tree.body if isinstance(n,ast.ClassDef) and n.name=='MidoLink')
    module=ast.fix_missing_locations(ast.Module(body=[copy.deepcopy(cls)],type_ignores=[]))
    fake=FakeMido()
    sys.modules['mido']=fake
    ns={'queue':queue,'time':time,'DELAY':{'open':0}}
    exec(compile(module,str(path),'exec'),ns)
    return ns['MidoLink'],fake

def run(path):
    Link, midi=load(path);checks=[]
    def ck(condition,description):
        checks.append((bool(condition),description))
        print(('PASS' if condition else 'FAIL'),description)
    b=Backend();l=Link(b,b.name,b.name)
    l.inp.inject(0x01); ck(l.read(0)==b'\xf0\x01\xf7' and not l.lost,'original unit receives own packet')
    # Attacker B appears at SAME OS name, enqueues hostile packet, A returns before poll.
    b.token='unit-B';l.inp.inject(0x42);b.token='unit-A'
    ck(l.read(0) is None,'foreign queued SysEx cannot survive physical B->A bounce')
    ck(l.lost,'physical B->A bounce leaves original link irreversibly tainted')
    ck(not l.send(b'\xf0\x09\xf7') and not l.out.sent,'future destructive send refused on bounced link')
    l.close()
    # Detect change through .lost and reject even after A reappears.
    b=Backend();l=Link(b,b.name,b.name)
    b.token='unit-B';ck(l.lost,'B replacement detected')
    b.token='unit-A';ck(l.lost and not l.send(b'\xf0\x09\xf7'),'identity restored cannot reopen tainted session')
    l.close()
    # Fresh legitimate named loader port may reopen with original token.
    b=Backend();first=Link(b,b.name,b.name);first.close();b.name='Felucca Update'
    second=Link(b,b.name,b.name)
    ck(second.send(b'\xf0\x01\xf7') and not second.lost,'fresh authenticated A loader rename works')
    second.close()
    # If provider exists but loses token, refuse permanently
    b=Backend();l=Link(b,b.name,b.name);b.token=None;ck(l.lost,'unavailable physical identity lost')
    b.token='unit-A';ck(l.lost,'provider recovery cannot untaint previous link')
    l.close()
    print('SUMMARY',sum(c for c,_ in checks),'/',len(checks),'passed')
    return not all(c for c,_ in checks)
if __name__=='__main__':
    src=sys.argv[1] if len(sys.argv)>1 else 'tools/fm1_install.py'
    sys.exit(run(src))
