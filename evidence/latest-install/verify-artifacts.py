#!/usr/bin/env python3
# Check the exact DEV package and controller. No device access occurs.
from pathlib import Path
import hashlib, json, re, struct, subprocess, sys, zlib
ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'tools'))
import dev_validate as dv
import fm1_install as fi
import fm1pkg_make as pm
import add_ctl
CANDIDATE = json.loads((OUT/'elite-linux/record.json').read_text())['commit']
def identity(data):
    return {'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'crc32':f'{zlib.crc32(data):08x}'}
result_path = OUT/'artifact-verification.json'
results = json.loads(result_path.read_text()) if result_path.exists() else {}
models = sys.argv[1:] or ['elite']
for model, product in [('elite', 'SMK-37 Elite_900'), ('pro','SMK-37 Pro_900')]:
    if model not in models:
        continue
    folder = OUT / 'elite-linux'
    record = json.loads((folder / 'record.json').read_text())
    pkg = folder / record['artifacts']['package']['file']
    ctl = folder / record['artifacts']['controller']['file']
    dv.verify(ROOT, CANDIDATE, folder / 'record.json', pkg, ctl)
    raw = pkg.read_bytes(); image, blocks = fi.deinterleave(raw)
    assert blocks == 20 and fi.product_of(raw) == product
    assert fi.package_damage(image) is None
    ctrl = ctl.read_bytes(); assert add_ctl.ctl_problem(ctrl) is None
    slots = pm.slotmap_of(image); assert slots == pm.SLOTMAP['smk37']
    st = add_ctl._open(image)
    assert len(st['flash']) == 0x9D000
    app_size = int(re.search(r'ok    image (\d+) B;', (folder/'build.log').read_text())[1])
    app = bytes(st['area'][0x120:0x120+app_size])
    assert app_size <= add_ctl.CODE_MAX
    assert st['area'][0x120+app_size:0x120+add_ctl.CODE_MAX] == b'\xff'*(add_ctl.CODE_MAX-app_size)
    assert add_ctl.controller_of(image) is None
    hdr = fi._dec(image[:0x40]); count = int.from_bytes(hdr[8:10],'little')
    entries = {}
    for i in range(count):
        e = fi._dec(image[0x40+i*0x50:0x40+(i+1)*0x50])
        off,size = struct.unpack_from('<II',e,8)
        name = e[0x40:0x50].split(b'\0')[0].decode('ascii')
        entries[name] = identity(image[off:off+size])
    assembled = add_ctl.add(raw, ctrl)
    assembled_image, assembled_blocks = fi.deinterleave(assembled)
    assert assembled_blocks == blocks and fi.package_damage(assembled_image) is None
    assert add_ctl.controller_of(assembled_image) == ctrl
    assert pm.slotmap_of(assembled_image) == slots
    after = add_ctl._open(assembled_image)
    assert after['flash'][:0x4000] == st['flash'][:0x4000]
    assert after['area'][0x120:0x120+app_size] == app
    assert add_ctl.add(assembled, None) == raw
    with (folder/'package-layout.log').open('w') as log:
        subprocess.run([sys.executable, str(ROOT/'tests/smk_pkg_test.py'), str(pkg), product],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
    results[model] = {'result':'PASS','record':str(folder/'record.json'),'validation_input_sha256':record['validation_inputs']['sha256'],'budgets':record['budgets'],'package':identity(raw),'controller':identity(ctrl),'controller_full_64k':identity(ctrl + b'\xff'*(65536-len(ctrl))),'app':identity(app),'entries':entries,'slot_map':slots,'assembled_install_image':identity(assembled),'checks':['The DEV record matches the candidate, logs, package and exact controller.','The package model, CRCs, region table and slot map pass.','The exact controller passes its CRC, ABI, RAM, entry and IRQ checks.','Controller insertion preserves app bytes, the bootloader head and slot map.','Controller removal restores the original package byte for byte.'],'limits':['The assembled image remains in memory. No device write occurs.','No physical-device PASS is established.']}
(OUT/'artifact-verification.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps({k:{'result':v['result'],'budgets':v['budgets']} for k,v in results.items()},indent=2))
