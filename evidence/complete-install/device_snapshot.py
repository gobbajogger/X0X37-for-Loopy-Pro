"""Read-only identity, complete slots and controller. No reset/settings/flash writes.
Uses maintained foreground and default hardware lock guards; run with a foreground TTY.
Only handshake, SMP_INFO, SMP_READ and CTL_READ are sent.
"""
import sys, json, hashlib, zlib, traceback
from pathlib import Path
from datetime import datetime, timezone, timedelta
out = Path(__file__).resolve().parent
lane = out.parents[1]
sys.path.insert(0,str(lane/'tools'))
import fm1_install as f, x0x_slots as xs

def fp(b):
    return dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest(),crc32=f'{zlib.crc32(b):08x}')

def snapshot(phase):
    dest = out/phase
    dest.mkdir(exist_ok=False)
    up = f.Updater(f.MidoBackend(),'X0X FM-1')
    dev = up.find()
    if not dev:
        raise RuntimeError('No matching device answered read-only handshake')
    try:
        assert not dev.id.loader, 'Running firmware required; no recovery/write authorized'
        assert dev.id.text == 'SMK-37 Elite_900', f'Unexpected model {dev.id.text}; stop for model decision'
        el = f.EditorLink(dev.link)
        lay, images = xs.backup(el)
        assert xs.flash_map(lay,'smk') == [(0xc2000,0x14000),(0xd6000,0x1e000),(0xb0000,0xb000)]
        arch = xs.make_archive(lay,images)
        (dest/'loops.x0xslots').write_bytes(arch)
        assert xs.read_archive((dest/'loops.x0xslots').read_bytes())[1] == images
        rows = []
        for k,s in enumerate(lay['slots']):
            full = xs.read_bytes(el,k,0,s['size'])
            path = dest/f'slot{k+1}-full.bin'
            path.write_bytes(full)
            assert path.read_bytes() == full
            if images[k] is not None:
                assert full[:len(images[k])] == images[k]
            rows.append(dict(slot=k+1,path=str(path),flash=s['off'],zones=s['nz'],
                             complete_slot=fp(full),loop_image=fp(images[k]) if images[k] is not None else None))
            print(phase,'slot',k+1,rows[-1]['complete_slot'],flush=True)
        controller = f.read_ctl(el,65536)
        (dest/'controller-64KiB.bin').write_bytes(controller)
        ctl = Path('/Users/mac/Documents/x0x37/smc_complete_stage4_20261021/evidence/complete-stage4/elite/ctl.img')
        expected = ctl.read_bytes()+b'\xff'*(65536-ctl.stat().st_size)
        assert controller == expected, 'Current controller differs; do not authorize silent replacement'
        rec = dict(identity=dev.id.text,port=dev.name,layout=lay,slots=rows,
                   archive=str(dest/'loops.x0xslots'),archive_fingerprint=fp(arch),
                   controller=fp(controller),controller_byte_equal=True,
                   time=datetime.now(timezone(timedelta(hours=11))).isoformat(),
                   commands='identity handshake, SMP_INFO15, SMP_READ16, CTL_READ37 only',device_writes=False)
        (dest/'snapshot.json').write_text(json.dumps(rec,indent=2)+'\n')
        print('READ-ONLY SNAPSHOT PASS:',dev.id.text,dev.name,'controller',fp(controller),flush=True)
        return rec
    finally:
        dev.link.close()

if __name__ == '__main__':
    if f.foreground_guard() or f.hardware_lock():
        sys.exit(10)
    try:
        snapshot(sys.argv[1])
    except BaseException:
        (out/'snapshot-error.txt').write_text(traceback.format_exc())
        raise
