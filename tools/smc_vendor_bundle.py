#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Explicit Elite central-only DEV bundle integrity and per-unit conversion.

No serial/MIDI I/O, SDK lookup, release authorization or implicit file writes.
The caller must bind loader_binding to its reviewed build record: it records the
app pin compiled into this exact loader, not a claim inferred from machine code.
CRC/SHA checks provide identity/integrity, not third-party authentication.
"""
import hashlib
import struct
import zlib

import add_ctl
import fm1pkg_make as pm

PROFILE = 'smc-vendor-central-dev'
MODEL = 'SMK-37 Elite'
PRODUCT = MODEL + '_900'
MODEL_ID = zlib.crc32(MODEL.encode())
APP_OFF, APP_MAX = 0x4120, 0x7FEE0
MAIN_OFF, MAIN_SIZE, CRYPT_SIZE = 0x4000, 0x99000, 0x9796E
PIECES = ((MAIN_OFF, MAIN_SIZE), (0xAB000, 8192), (0xAD000, 8192), (0xFE000, 4096))
REGIONS = ((0x84000, 94208), (0xAD000, 8192), (0xFE000, 4096))
BASELINE_FWSC_SHA = '6f89a299deb2304a89e3094f25ba22dd70214a3ce935c770a6e2ccff39e9b65a'
BASELINE_MAIN_SHA = '55e9e093d4adc7a2b303a807873cae536c6547013dd2c36af152ba92653ce446'
BASELINE_APP_SHA = '4b7c76e4bdeca3d8b1ce366e868406b8d554b0ae7d87beb282bd99254dcd0ec9'
BASELINE_CTL_SHA = '9d6855e47791b3cab698af95d9625c35d2ea7b01d1fa7216e72ea622683a8245'
BINDING_KEYS = {'app_size', 'app_sha256', 'app_crc32', 'descriptor_off', 'ota_sha256'}


class BundleError(ValueError):
    pass


def need(ok, message):
    if not ok:
        raise BundleError(message)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def _entry(b):
    d = add_ctl._dec(b)
    need(len(d) == 32 and pm.crc16(d[2:]) == struct.unpack_from('<H', d)[0], 'JLFS entry CRC')
    return d


def _head_entries(head):
    """The supported fixed two-file head layout; no writes or key repair."""
    need(len(head) == 0x4000, 'exact 16 KiB unit head required')
    _entry(head[:32])
    spl, isd = _entry(head[32:64]), _entry(head[64:96])
    for e, name in ((spl, b'uboot.boot'), (isd, b'isd_config.ini')):
        need(e[16:32].rstrip(b'\0\xff') == name, 'unexpected head entry')
        off, size = struct.unpack_from('<II', e, 4)
        need(0xA0 <= off <= 0x4000 and 0 < size <= 0x4000-off, 'head entry outside head')
    return spl, isd


def _key_blob(head, isd):
    off, size = struct.unpack_from('<II', isd, 4)
    need(size >= 34 and pm.crc16(head[off:off+32]) == struct.unpack_from('<H', head, off+32)[0], 'key blob CRC')
    return pm.chipkey_decode(head[off:off+32])


def head_key(head):
    """Strict generated/source-template contract, including both payload CRCs."""
    spl, isd = _head_entries(head)
    for e in (spl, isd):
        off, size = struct.unpack_from('<II', e, 4)
        need(pm.crc16(head[off:off+size]) == struct.unpack_from('<H', e, 2)[0], 'head data CRC')
    return _key_blob(head, isd)


def physical_head_key(head, *, diagnostics=None):
    """Native key contract for an immutable physical readback, NOT boot validation.

    Only the pinned native bundle route may use this key: it re-reads the live
    key and never writes the head. Whole SPL/isd payloads are opaque preserved
    bytes, not installed components. Their CRCs remain visible diagnostics.
    CRC-valid keys are not independently attested against hardware/old app.
    """
    spl, isd = _head_entries(head)
    # Match ldr_chip_key's traversal/name semantics, in addition to host bounds.
    need(struct.unpack_from('<H', spl, 14)[0] == 0, 'native key path ends before isd_config.ini')
    need(isd[16:31] == b'isd_config.ini\0', 'native key entry name')
    key = _key_blob(head, isd)
    if diagnostics is not None:
        rows = []
        for e, name in ((spl, 'uboot.boot'), (isd, 'isd_config.ini')):
            off, size = struct.unpack_from('<II', e, 4)
            expected = struct.unpack_from('<H', e, 2)[0]
            actual = pm.crc16(head[off:off+size])
            rows.append(dict(entry=name, offset=off, size=size, expected_crc16=expected,
                             actual_crc16=actual, matches=expected == actual))
        diagnostics(dict(contract='native-key-preserved-head-v1', key_blob_crc_valid=True,
                         payload_crc_admission=False, payloads=rows))
    return key


def unpack(raw):
    """Strict existing UFW container: exactly flash.bin and paired ota.bin."""
    try:
        product, lg = add_ctl.logical_of(raw)
    except add_ctl.PkgError as e:
        raise BundleError(str(e)) from e
    need(product == PRODUCT, 'Elite DEV product identity required')
    h = add_ctl._dec(lg[:64])
    need(len(h) == 64 and pm.crc16(h[2:]) == struct.unpack_from('<H', h)[0], 'UFW header CRC')
    need(struct.unpack_from('<I', h, 4)[0] == len(lg), 'UFW exact total length')
    need(struct.unpack_from('<H', h, 8)[0] == 2 and h[16:22] == b'AC791N', 'UFW entry count/chip')
    need(pm.crc16(lg[64:224]) == struct.unpack_from('<H', h, 2)[0], 'UFW file list CRC')
    entries, spans = {}, []
    for i in range(2):
        e = add_ctl._dec(lg[64+80*i:144+80*i])
        typ, idx, crc, reserved, off, size, padded = struct.unpack_from('<HHHHIII', e)
        need(typ in (0, 100) and typ not in entries and idx == i and not reserved, 'UFW entry identity')
        need(e[64:80].rstrip(b'\0\xff') == (b'flash.bin' if typ == 0 else b'ota.bin'), 'UFW entry name')
        need(off >= 0x400 and not off % 256 and 0 < size <= len(lg)-off and padded == (size+31)&~31, 'UFW entry range')
        spans.append((off, off+size))
        b = bytes(lg[off:off+size])
        need(pm.crc16(b) == crc, 'UFW payload CRC')
        entries[typ] = b
    spans.sort()
    need(spans[0][1] <= spans[1][0] and spans[-1][1] == len(lg), 'UFW overlapping/trailing payload')
    return entries[0], entries[100]


def _main(flash, *, physical_head=None):
    need(len(flash) >= 0x9D000, 'short flash template')
    if physical_head is None:
        key = head_key(flash[:0x4000])
    else:
        need(flash[:0x4000] == physical_head, 'bound physical head changed')
        key = physical_head_key(physical_head)
    m = bytearray(flash[MAIN_OFF:MAIN_OFF+MAIN_SIZE])
    pm.sfc(m, 0, CRYPT_SIZE, 0, key)
    blk = struct.unpack_from('<I', m, 8)[0]
    need(m[16:32].rstrip(b'\0\xff') == b'app_area_head' and struct.unpack_from('<I', m, 4)[0] == 0x02000120, 'app area identity')
    need(0x97520 <= blk <= CRYPT_SIZE-32, 'app block bound')
    need(struct.unpack_from('<II', m, 36) == (0x120, 0x97400) and m[48:64].rstrip(b'\0\xff') == b'app.bin', 'app slot identity')
    need(pm.crc16(m[2:32]) == struct.unpack_from('<H', m)[0] and pm.crc16(m[32:blk]) == struct.unpack_from('<H', m, 2)[0], 'app area CRC')
    need(pm.crc16(m[34:64]) == struct.unpack_from('<H', m, 32)[0] and pm.crc16(m[0x120:0x97520]) == struct.unpack_from('<H', m, 34)[0], 'app entry CRC')
    need(m[blk+16:blk+32].rstrip(b'\0\xff') == b'cfg' and blk+struct.unpack_from('<I', m, blk+8)[0] == CRYPT_SIZE, 'exact supported encrypted cfg extent')
    expected = pm.slotmap_blob(pm.SLOTMAP['smk37'])
    need(m[0x98F00:0x98F00+len(expected)] == expected, 'exact sample slot map required')
    return m, key, blk


def _recrc(m, blk):
    struct.pack_into('<H', m, 34, pm.crc16(m[0x120:0x97520]))
    struct.pack_into('<H', m, 32, pm.crc16(m[34:64]))
    struct.pack_into('<H', m, 2, pm.crc16(m[32:blk]))
    struct.pack_into('<H', m, 0, pm.crc16(m[2:32]))


def _loader(ota, binding):
    need(544 <= len(ota) <= 0x4000 and sha(ota) == binding['ota_sha256'], 'paired loader identity/size')
    need(pm.crc16(ota[2:32]) == struct.unpack_from('<H', ota)[0] and pm.crc16(ota[34:64]) == struct.unpack_from('<H', ota, 32)[0], 'loader header CRC')
    off, n = struct.unpack_from('<II', ota, 4)
    need(off == 32 and n+32 == len(ota) and n >= 128, 'loader framing')
    need(pm.crc16(ota[32:]) == struct.unpack_from('<H', ota, 2)[0], 'loader payload CRC')
    need(struct.unpack_from('<I', ota, 40)[0] == 0x01C0A800 and 0 < struct.unpack_from('<I', ota, 36)[0] <= 0x14000, 'loader RAM bound')
    need(b'X0X-BUNDLE-LDR1\0' in ota[0x48:0x88], 'bundle loader capability')


def _binding(binding):
    need(isinstance(binding, dict) and set(binding) == BINDING_KEYS, 'exact loader binding fields required')
    need(type(binding['app_size']) is int and 20 <= binding['app_size'] <= APP_MAX, 'app size bound')
    need(type(binding['descriptor_off']) is int and APP_OFF <= binding['descriptor_off'] <= APP_OFF+binding['app_size']-20 and binding['descriptor_off'] % 4 == 0, 'descriptor in actual app')
    need(type(binding['app_crc32']) is int and 0 <= binding['app_crc32'] < 2**32, 'app CRC range')
    for k in ('app_sha256', 'ota_sha256'):
        need(isinstance(binding[k], str) and len(binding[k]) == 64 and all(c in '0123456789abcdef' for c in binding[k]), 'SHA256 identity')


def _manifest(regions):
    need(len(regions) == 3, 'exactly three complete vendor regions required')
    for b, (_, n) in zip(regions, REGIONS):
        need(len(b) == n, 'complete padded vendor region required')
    h = struct.unpack_from('<IHH30I', regions[0])
    need(h[:8] == (0x53423058, 1, 128, 4, MODEL_ID, 0x534D4302, 1, h[7]) and h[7] and not any(h[31:]), 'ABI4 manifest identity')
    need(zlib.crc32(regions[0][:116]+b'\0'*4+regions[0][120:128]) == h[30], 'ABI4 manifest CRC')
    for i, (b, (off, cap)) in enumerate(zip(regions, REGIONS)):
        so, n, ex, crc = h[18+4*i:22+4*i]
        need(so == off and 0 < n <= cap and n % 4 == ex % 4 == 0 and ex <= n and (ex >= 132 if i == 0 else ex == 0), 'ABI4 segment extent')
        need(zlib.crc32(b[128 if i == 0 else 0:n]) == crc and b[n:] == b'\xff'*(cap-n), 'ABI4 payload CRC/padding')
        if i:
            for p in range(0xF00, cap, 4096):
                need(b[p+6:p+8] != b'AT', 'secondary resembles update record')
    for a in h[8:10]:
        need(a % 2 == 0 and 0x02080080 <= a < 0x02080000+h[20], 'ABI4 executable entry')
    sl, sh, rl, rh, dl, dh, il, ih = h[10:18]
    need(rl == 0x01C01800 and rl <= sl < sh <= rh <= 0x01C07F00 and (sl|sh|rh) % 16 == 0 and dl == 0x01C7D400 <= dh <= 0x01C7F000 and dh % 16 == 0, 'ABI4 RAM/stack guard')
    need(not il & sum(1 << x for x in (1,11,16,20,24)) and not ih & sum(1 << x for x in (5,30,31)), 'ABI4 reserved IRQ')
    return h[7]


def _suffix(f, descriptor_off, image_id, policy=1):
    h = [0x31425058,1,MODEL_ID,policy,0x84000 if policy == 1 else 0x8B000,128,0,0,descriptor_off,image_id,0,0]
    aggregate = 0
    for off, n in PIECES:
        b = f[off:off+n]
        h.extend((off,n,zlib.crc32(b)))
        aggregate = zlib.crc32(b, aggregate)
    h[6] = aggregate
    h[7] = zlib.crc32(struct.pack('<24I', *h))
    return struct.pack('<24I', *h)


def _info(off, b):
    return dict(offset=off, size=len(b), sha256=sha(b), crc32=zlib.crc32(b))


def _validate_bundle(raw, *, loader_binding, physical_head=None):
    _binding(loader_binding)
    f, ota = unpack(raw)
    _loader(ota, loader_binding)
    need(len(f) == 0x100060, 'whole-bundle flash plus exact 96-byte record required')
    m, _, _ = _main(f, physical_head=physical_head)
    policy = struct.unpack_from('<I', f, 0x100000+12)[0]
    need(policy in (1,2), 'unsupported bundle policy')
    if policy == 2:
        return _validate_rollback(raw, f, m, loader_binding)
    app = m[0x120:0x120+loader_binding['app_size']]
    need(sha(app) == loader_binding['app_sha256'] and zlib.crc32(app) == loader_binding['app_crc32'], 'app does not match paired loader pin')
    need(m[0x120+len(app):0x80000] == b'\xff'*(APP_MAX-len(app)), 'non-erased app tail')
    regions = [bytes(m[0x80000:0x97000]), f[0xAD000:0xAF000], f[0xFE000:0xFF000]]
    image_id = _manifest(regions)
    ui = f[0xAB000:0xAD000]
    desc = struct.unpack_from('<5I', m, loader_binding['descriptor_off']-MAIN_OFF)
    need(desc == (0x55493058,1,0xAB000,8192,zlib.crc32(ui)), 'app-owned UI descriptor binding')
    for p in (0xABF00,0xACF00):
        need(f[p+6:p+8] != b'AT', 'app UI resembles update record')
    need(f[0x100000:] == _suffix(f, loader_binding['descriptor_off'], image_id), 'whole-bundle metadata identity/CRC')
    return dict(profile=PROFILE, model=MODEL, product=PRODUCT, policy=1,
                package_sha256=sha(raw), canonical_main_sha256=sha(m), **loader_binding,
                controller=dict(kind='segmented-abi4', image_id=image_id,
                                regions=[_info(o,b) for (o,_),b in zip(REGIONS,regions)]),
                ui=_info(0xAB000,ui))


def validate_bundle(raw, *, loader_binding):
    """Strict SOURCE/generated bundle validation; no physical-head exceptions."""
    return _validate_bundle(raw, loader_binding=loader_binding)


def validate_bound_bundle(raw, *, source_raw, actual_head, loader_binding):
    """Explicit native-only per-unit result; exact immutable head is mandatory.

    The strict source bytes are required again: physical-head admission cannot
    turn an arbitrary converted package into independently checked source.
    The caller still needs the checked DEV record for the loader build recipe.
    """
    need(type(actual_head) is bytes, 'immutable bound physical head required')
    need(type(source_raw) is bytes, 'immutable strict source package required')
    before = validate_bundle(source_raw, loader_binding=loader_binding)
    after = _validate_bundle(raw, loader_binding=loader_binding, physical_head=actual_head)
    need({k:v for k,v in before.items() if k != 'package_sha256'} ==
         {k:v for k,v in after.items() if k != 'package_sha256'},
         'conversion changed canonical component identity')
    return after


def fixed_slot_map():
    """Map already checked at flash 0x9CF00 by _main; never scan opaque head."""
    return list(pm.SLOTMAP['smk37'])



def _validate_rollback(raw, f, m, loader_binding):
    """Exact logical baseline restore, not original spare-byte restoration."""
    app = m[0x120:0x120+548480]
    block = bytes(m[0x87000:0x97000])
    ctl = block[:65524]
    need(sha(m) == BASELINE_MAIN_SHA and zlib.crc32(m) == 0x53F84577, 'rollback requires exact approved canonical baseline main')
    need(sha(app) == BASELINE_APP_SHA and sha(ctl) == BASELINE_CTL_SHA and add_ctl.ctl_problem(ctl) is None and block[65524:] == b'\xff'*12, 'rollback approved app/ABI2 pair')
    for off,n in PIECES[1:]:
        need(f[off:off+n] == b'\xff'*n, 'logical rollback must clear only proven unused secondary extents')
    need(f[0x100000:] == _suffix(f,0,0x0816E0D3,2), 'rollback metadata identity/CRC')
    return dict(profile=PROFILE,model=MODEL,product=PRODUCT,policy=2,
                package_sha256=sha(raw),canonical_main_sha256=sha(m),
                app_size=len(app),app_sha256=sha(app),app_crc32=zlib.crc32(app),descriptor_off=0,
                ota_sha256=loader_binding['ota_sha256'],paired_loader_app=dict(loader_binding),
                controller=dict(kind='baseline-abi2',image_id=0x0816E0D3,regions=[_info(0x8B000,block)]),
                cleared_regions=[_info(off,f[off:off+n]) for off,n in PIECES[1:]])


def build_rollback(approved_baseline_fwsc, paired_ota, *, loader_binding):
    """Package the pinned 816 pair with the SAME bundle loader, never old updater.

    Clears AB000..AEFFF and FE000..FEFFF (unused by the baseline). Samples,
    owner tag, calibration, key and head are untouched. Not byte-for-byte rollback.
    """
    _binding(loader_binding)
    _loader(paired_ota,loader_binding)
    need(sha(approved_baseline_fwsc) == BASELINE_FWSC_SHA, 'exact approved 816 baseline package required')
    st = add_ctl._open(add_ctl.logical_of(approved_baseline_fwsc)[1])
    f = bytearray(b'\xff'*0x100000)
    need(len(st['flash']) == 0x9D000, 'baseline flash extent')
    f[:len(st['flash'])] = st['flash']
    f += _suffix(f,0,0x0816E0D3,2)
    raw = pm.ufw(f,paired_ota,PRODUCT)
    validate_bundle(raw,loader_binding=loader_binding)
    return raw

def build_bundle(flash_template, sealed_app, ui, vendor_regions, ota, *, descriptor_off, loader_binding):
    """Construct DEV bytes; caller controls explicit publication after review."""
    _binding(loader_binding)
    need(descriptor_off == loader_binding['descriptor_off'], 'descriptor provenance mismatch')
    need(len(flash_template) == 0x9D000, 'exact supported pm.flash_image template required')
    need(len(sealed_app) == loader_binding['app_size'] and sha(sealed_app) == loader_binding['app_sha256'], 'sealed app binding')
    need(len(ui) == 8192, 'complete UI extent required')
    image_id = _manifest(vendor_regions)
    m, key, blk = _main(flash_template)
    need(m[0x120:0x120+len(sealed_app)] == sealed_app and m[0x120+len(sealed_app):0x97520] == b'\xff'*(0x97400-len(sealed_app)), 'template must contain only exact sealed app and erased controller slot')
    m[0x80000:0x97000] = vendor_regions[0]
    _recrc(m, blk)
    pm.sfc(m, 0, CRYPT_SIZE, 0, key)
    f = bytearray(b'\xff'*0x100000)
    f[:len(flash_template)] = flash_template
    f[MAIN_OFF:MAIN_OFF+MAIN_SIZE] = m
    f[0xAB000:0xAD000] = ui
    f[0xAD000:0xAF000] = vendor_regions[1]
    f[0xFE000:0xFF000] = vendor_regions[2]
    f += _suffix(f, descriptor_off, image_id)
    raw = pm.ufw(f, ota, PRODUCT)
    validate_bundle(raw, loader_binding=loader_binding)
    return raw


def bind_head(raw, actual_head, *, loader_binding, diagnostics=None):
    """Convert encryption to this unit; preserve its entire readback head exactly."""
    before = validate_bundle(raw, loader_binding=loader_binding)
    f, ota = unpack(raw)
    m, _, _ = _main(f)
    need(type(actual_head) is bytes, 'immutable physical head required')
    key = physical_head_key(actual_head, diagnostics=diagnostics)
    pm.sfc(m, 0, CRYPT_SIZE, 0, key)
    f = bytearray(f[:0x100000])
    f[:0x4000] = actual_head
    f[MAIN_OFF:MAIN_OFF+MAIN_SIZE] = m
    f += _suffix(f, before['descriptor_off'], before['controller']['image_id'], before['policy'])
    converted = pm.ufw(f, ota, PRODUCT)
    after = validate_bound_bundle(converted, source_raw=raw, actual_head=actual_head, loader_binding=loader_binding)
    need({k:v for k,v in before.items() if k != 'package_sha256'} == {k:v for k,v in after.items() if k != 'package_sha256'}, 'conversion changed canonical component identity')
    receipt = dict(schema=1, profile=PROFILE, source_sha256=sha(raw), converted_sha256=sha(converted),
                   head_sha256=sha(actual_head), loader_binding=dict(loader_binding), components=after)
    return converted, receipt


def response_bytes(image, flags, address, count, expected_finish):
    """Immutable existing uploader read policy; reject before responding."""
    need(type(image) is bytes, 'immutable response image required')
    need(flags == 0 and type(count) is int and 0 < count <= 512 and type(address) is int and 0 <= address <= 0xFFFFFFFF, 'invalid request flags/length/address')
    need(expected_finish in (0xE0000000,0xF0000000), 'invalid serving phase')
    if address in (0xE0000000,0xF0000000):
        need(address == expected_finish and count == 8, 'unexpected finish phase/length')
        return b'success\0', True
    need(address <= len(image) and count <= len(image)-address, 'source request outside immutable package')
    return image[address:address+count], False
