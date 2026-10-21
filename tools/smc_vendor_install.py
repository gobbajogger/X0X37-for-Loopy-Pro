# SPDX-License-Identifier: GPL-3.0-only
"""Opt-in adapter for fm1_install's existing transport, locks and slot helpers.
No alternate uploader. No release authorization or automatic cold/RF claim.
"""
import json
import os
from pathlib import Path
import tempfile

import smc_vendor_bundle as bundle


def prerequisites(a):
    bundle.need(a.config == 'dev' and a.dev_record and a.commit, 'SMC DEV requires exact commit and scoped DEV record')
    bundle.need(not (a.ctl or a.no_ctl or a.force or a.no_slots), 'SMC DEV forbids --ctl/--no-ctl/--force/--no-slots')
    bundle.need(a.backup and a.verify_slots and a.smc_receipt and a.smc_cold_recovery, 'SMC DEV requires --backup, --verify-slots, --smc-receipt and explicit --smc-cold-recovery')
    bundle.need(a.head_ref, 'SMC DEV requires explicit --head-ref from this physical unit')
    paths = [(name, Path(getattr(a,name))) for name in
             ('package','dev_record','head_ref','backup','smc_receipt')]
    for i,(name,path) in enumerate(paths):
        for other,q in paths[:i]:
            same = path.resolve() == q.resolve()
            if path.exists() and q.exists():
                same = same or os.path.samefile(path,q)
            bundle.need(not same, f'{name} must not alias {other}; preserve package, head, evidence and backup')


def prepare(a):
    """Read once, validate record identities, convert in memory, freeze responses."""
    prerequisites(a)
    if hasattr(a, '_smc_prepared'):
        return a._smc_prepared
    record_bytes = Path(a.dev_record).read_bytes()
    if hasattr(a, '_smc_record_sha'):
        bundle.need(bundle.sha(record_bytes) == a._smc_record_sha, 'DEV record changed after gate verification')
    record = json.loads(record_bytes)
    bundle.need(record.get('schema') == 3 and record.get('result') == 'DEV CHECKED' and record.get('profile') == bundle.PROFILE and record.get('model') == 'elite' and record.get('config') == 'dev', 'explicit Elite segmented DEV record required')
    raw = Path(a.package).read_bytes()
    binding = record['loader_binding']
    meta = bundle.validate_bundle(raw, loader_binding=binding)
    bundle.need((meta['policy'] == 2) == bool(a.smc_logical_rollback), 'logical rollback requires its explicit selector; forward package must not use it')
    bundle.need(meta == record['smc_vendor'], 'package/components differ from checked DEV record')
    # A full-unit readback may be supplied; only the immutable 16 KiB head is used.
    with open(a.head_ref, 'rb') as f:
        head = f.read(0x4000)
    converted, receipt = bundle.bind_head(raw, head, loader_binding=binding)
    product, logical = bundle.add_ctl.logical_of(converted)
    receipt.update(dev_record_sha256=bundle.sha(record_bytes), source_commit=a.commit)
    a._smc_prepared = (product, bytes(logical), receipt, meta)
    return a._smc_prepared


def save_receipt(path, data):
    """Durable local transaction identity BEFORE any UPGRADE request."""
    path = Path(path)
    encoded = (json.dumps(data, sort_keys=True, indent=2)+'\n').encode()
    fd, tmp = tempfile.mkstemp(prefix=path.name+'.', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as f:
            f.write(encoded); f.flush(); os.fsync(f.fileno())
        os.replace(tmp, path)
        d = os.open(path.parent, os.O_RDONLY)
        try: os.fsync(d)
        finally: os.close(d)
    finally:
        if os.path.exists(tmp): os.unlink(tmp)


def read_region(ed, region, size, fi):
    got = bytearray()
    while len(got) < size:
        off, n = len(got), min(224, size-len(got))
        args = [region,off&127,(off>>7)&127,(off>>14)&127,(n+1)//2]
        r = fi.xs.req(ed,39,args,fi.CTL_READ['timeout'],fi.CTL_READ['tries'])
        if len(r) < 4 or list(r[:4]) != args[:4] or any(not 0 <= x < 128 for x in r):
            raise fi.InstallError('ctlcrc', 'foreign or malformed ED_SMC_READ reply')
        data = fi.xs.unpack7(r[4:])
        if len(data) != n or fi.xs.pack7(data) != list(r[4:]):
            raise fi.InstallError('ctlcrc', 'short/oversized ED_SMC_READ reply')
        got.extend(data)
    return bytes(got)


def check_regions(link, meta, out, fi):
    if meta['policy'] == 2:
        # Baseline has ABI2 ED37, not ED39. The RAM loader already read-verified
        # every cleared spare byte; do not claim a postboot spare read here.
        want = meta['controller']['regions'][0]
        got = fi.read_ctl(fi.EditorLink(link),0x10000)
        if bundle.sha(got) != want['sha256']:
            raise fi.InstallError('ctlcrc','logical rollback ABI2 block mismatch')
        print('Logical rollback: complete ABI2 block matched; cleared spares were loader-verified, not postboot ED39-read',file=out)
        return
    expected = [meta['controller']['regions'][0],meta['ui'],*meta['controller']['regions'][1:]]
    ed = fi.EditorLink(link)
    for i, want in enumerate(expected):
        got = read_region(ed,i,want['size'],fi)
        if bundle.sha(got) != want['sha256']:
            raise fi.InstallError('ctlcrc', f'installed region {i} differs from exact DEV package; no repair attempted')
        print(f'SMC region {i}: all {len(got)} bytes read back and matched',file=out)


def cold_state(reply):
    """Exact supported DIAG layouts; OFF-after-start must not pass.
    Raw reset cause is retained, not interpreted as proof of a physical cycle.
    """
    bundle.need(reply and all(type(x) is int and 0 <= x < 128 for x in reply), 'malformed cold-state reply')
    n=reply[0]
    bundle.need(len(reply)==1+5*n and all(reply[5+5*i]<16 for i in range(n)), 'invalid DIAG word framing')
    w=[sum(reply[1+5*i+j] << (7*j) for j in range(5)) for i in range(n)]
    if n==63 and w[-3]==0x534D4347:
        bundle.need(w[-2]==1, 'TEST radio attempted, requested ON, or recovery not admitted; save OFF and power cycle')
        return dict(layout='vendor-update-gate-v1',flags=w[-2],reset_raw=w[-1])
    bundle.need(n==71 or (n==77 and w[-6]==0x534D4301), 'unsupported DIAG layout; no guessed cold-state admission')
    # 17 crash + 2 bootguard + 29 retained guard words; current live flags follow.
    flags,want,state=w[48],w[49],w[51]
    bundle.need(flags & 0xFFFFFF00 == 0 and want & 0xFFFF == 0 and state==1,
                'controller started or Bluetooth requested ON; save OFF and physically power cycle')
    return dict(layout='abi2-live-v1',flags=flags,want=want,reset_raw=w[58])

def check_cold(link,fi):
    return cold_state(list(fi.xs.req(fi.EditorLink(link),33,[],fi.CTL_READ['timeout'],fi.CTL_READ['tries'])))


def run(a, backend, out, ask, fi):
    """Use Updater, SlotMove, same-port binding and existing MIDI backend only."""
    try:
        product,image,identity,meta = prepare(a)
        class EliteUpdater(fi.Updater):
            def find(self, want=None, only_port=True):
                dev = super().find(want,only_port)
                if dev and fi.model_of(dev.id.text) != bundle.MODEL:
                    dev.link.close()
                    raise fi.InstallError('model','SMC vendor DEV supports the exact SMK-37 Elite only')
                return dev
        up = EliteUpdater(backend,a.port)
        up.read_policy = bundle.response_bytes
        dev = up.find()
        if not dev: raise fi.not_found(up)
        up.bind(dev)
        rp = Path(a.smc_receipt)
        resuming = rp.exists()
        if resuming:
            transaction = json.loads(rp.read_text())
            bundle.need(transaction.get('state') in ('prepared','writing','written','verified'), 'invalid transaction state')
            bundle.need(transaction.get('identity') == identity, 'resume package/head/loader/record identity mismatch')
            bundle.need(transaction.get('backup_path') == str(Path(a.backup).resolve()), 'resume backup path mismatch')
            backup = Path(a.backup).read_bytes()
            bundle.need(bundle.sha(backup) == transaction['backup_sha256'], 'resume backup changed')
            _,images = fi.xs.read_archive(backup)
            old = [tuple(x) for x in transaction['old_map']]
            new = [tuple(x) for x in transaction['new_map']]
            bundle.need(old == new == fi.package_slot_map(image,product), 'resume sample layout mismatch')
            move = fi.SlotMove(transaction['layout'],old,new,[],'smk')
            move.images,move.path = images,Path(a.backup)
            bundle.need(len(images) == len(old), 'resume backup slot count')
        else:
            bundle.need(not dev.id.loader, 'loader resume requires the original saved transaction receipt and backup')
            bundle.need(not Path(a.backup).exists(), 'refuse to overwrite an existing sample backup')
            check_cold(dev.link,fi)
            move = fi.plan_slots(dev,image,product,require_backup=True)
            bundle.need(move is not None and move.old_map == move.new_map and not move.risk, 'SMC DEV must preserve the exact sample layout')
            if not a.yes and not ask('Install central-only DEV from physical cold recovery? [y/N] '):
                dev.link.close(); return 1
            move.backup(dev.link,a.backup,out)
            # A durable receipt must never precede the backup it promises.
            with open(a.backup,'rb') as backup_file:
                os.fsync(backup_file.fileno())
            backup_dir = os.open(Path(a.backup).resolve().parent,os.O_RDONLY)
            try: os.fsync(backup_dir)
            finally: os.close(backup_dir)
            bundle.need(move.old_map == move.new_map and not move.risk, 'actual sample backup changed layout/risk')
            transaction = dict(identity=identity,backup_path=str(Path(a.backup).resolve()),
                               backup_sha256=bundle.sha(Path(a.backup).read_bytes()),
                               old_map=move.old_map,new_map=move.new_map,layout=move.lay,state='prepared')
            save_receipt(rp,transaction)
        print('SMC central-only DEV: USB to Loopy; physical cold recovery required. First migration relies on this operator prerequisite, not a gate in old firmware.',file=out)
        # If a completed write was recorded, verify only. A lost finish remains
        # 'writing'; rerunning uses the SAME immutable source and paired loader.
        if transaction['state'] not in ('written','verified'):
            if resuming and not a.yes and not ask('Continue the exact saved DEV transaction from cold recovery? [y/N] '):
                dev.link.close(); return 1
            if not dev.id.loader:transaction['cold_preflight']=check_cold(dev.link,fi)
            transaction['state']='writing';save_receipt(rp,transaction)
            prog=fi.Progress(out)
            try:up.install(dev,image,product,prog)
            finally:prog.end()
            transaction['state']='written';save_receipt(rp,transaction)
        else:
            bundle.need(not dev.id.loader and dev.id.text == product, 'completed receipt but expected app is not running')
            up.target_name,up.verified=dev.name,True
            dev.link.close()
        back=up.wait_for(lambda i:not i.loader,fi.DELAY['wait_reboot'])
        if not back:raise fi.InstallError('noreturn','bound keyboard did not answer for complete readback')
        try:
            bundle.need(back.id.text == product, 'postinstall firmware identity mismatch')
            check_regions(back.link,meta,out,fi)
            move.verify(back.link,out)
        finally:back.link.close()
        transaction['state']='verified';save_receipt(rp,transaction)
        return 0
    except (bundle.BundleError,KeyError,ValueError,OSError,fi.xs.SlotError,TimeoutError) as e:
        if 'dev' in locals():dev.link.close()
        raise fi.InstallError('badpkg',f'SMC DEV refused or incomplete: {e}; retain receipt/backup and exact package') from e
    finally:
        if 'dev' in locals():
            dev.link.close()
