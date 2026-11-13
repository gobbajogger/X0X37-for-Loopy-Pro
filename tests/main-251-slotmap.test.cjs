const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const vm = require('node:vm');

const html = fs.readFileSync(path.join(__dirname, '../install/index.html'), 'utf8');
const cat = JSON.parse(fs.readFileSync(path.join(__dirname, '../versions.json'), 'utf8'));

const PKG_SHA = {
  elite: '5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a',
  pro: '6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf',
};
const CTL_SHA = 'a334208b51f62bc7231ebb76a6de0dfa260033d7f7316e5b92053c540bc1d984';
const CURRENT = [[0xC2000, 0x14000], [0xD6000, 0x1E000], [0xB3000, 0x8000]];
const MAP = 0x9CF00;
const MAGIC = Buffer.from('X0X-SLOTMAP-1\0\0\0', 'binary');

function slice(start, end) {
  const i = html.indexOf(start), j = html.indexOf(end, i);
  assert.ok(i >= 0 && j > i, start);
  return html.slice(i, j);
}
const identStart = html.indexOf('function keyboardModel(model)');
const identEnd = html.indexOf('\n', html.indexOf('const sameKeyboard =', identStart)) + 1;
const ctx = { Uint8Array, TextDecoder, Error };
vm.runInNewContext(
  slice('const BLOCKS = 20', '\nfunction insertCtl(') + '\n' + html.slice(identStart, identEnd) +
  `\nthis.productOf=productOf;this.logicalImage=logicalImage;this.controllerOf=controllerOf;` +
  `this.pkHasSlotMap=pkHasSlotMap;this.pkOpenController=pkOpenController;` +
  `this.pkEnc=pkEnc;this.pkCrc16=pkCrc16;this.pkCrc32=pkCrc32;` +
  `this.pkU16=pkU16;this.pkU32=pkU32;this.pkPut16=pkPut16;` +
  `this.keyboardModel=keyboardModel;this.sameKeyboard=sameKeyboard;`,
  ctx
);
const { productOf, logicalImage, controllerOf, pkHasSlotMap, pkOpenController,
  pkEnc, pkCrc16, pkCrc32, pkU16, pkU32, pkPut16, keyboardModel, sameKeyboard } = ctx;

function pkPut32(d, o, v) {
  d[o] = v & 255; d[o + 1] = (v >> 8) & 255; d[o + 2] = (v >> 16) & 255; d[o + 3] = (v >>> 24) & 255;
}

function loadPkg(which) {
  const names = { elite: 'pkg-elite.fwsc', pro: 'pkg-pro.fwsc' };
  const candidates = [
    process.env['MAIN251_' + which.toUpperCase() + '_FWSC'],
    path.join('/workspace/kept-025-20261112/79280e763', names[which]),
    path.join(__dirname, 'fixtures/main-251', names[which]),
  ].filter(Boolean);
  const p = candidates.find((f) => fs.existsSync(f));
  assert.ok(p, '0.0.25.1 ' + which + ' package not found (sha ' + PKG_SHA[which] + ')');
  const buf = fs.readFileSync(p);
  assert.equal(crypto.createHash('sha256').update(buf).digest('hex'), PKG_SHA[which], p);
  return buf;
}

function writeMap(flash, slots) {
  flash.fill(0xFF, MAP, MAP + 64);
  if (!slots) return;
  flash.set(MAGIC, MAP);
  pkPut32(flash, MAP + 16, slots.length);
  slots.forEach(([off, size], j) => {
    pkPut32(flash, MAP + 20 + j * 8, off);
    pkPut32(flash, MAP + 24 + j * 8, size);
  });
  const end = MAP + 20 + 8 * slots.length;
  pkPut32(flash, end, pkCrc32(flash, MAP, end));
}

function resignLogical(lg) {
  const hdr = lg.slice(0, 0x40);
  pkEnc(hdr, 0, 0x40);
  const count = pkU16(hdr, 8);
  const lst = lg.slice(0x40, 0x40 + 0x50 * count);
  for (let j = 0; j < count; j++) {
    const entry = lst.slice(j * 0x50, (j + 1) * 0x50);
    pkEnc(entry, 0, 0x50);
    if (pkU16(entry, 0) === 0) {
      const off = pkU32(entry, 8), size = pkU32(entry, 12);
      pkPut16(entry, 4, pkCrc16(lg, off, off + size));
      pkEnc(entry, 0, 0x50);
      lst.set(entry, j * 0x50);
      break;
    }
  }
  pkPut16(hdr, 2, pkCrc16(lst));
  pkPut16(hdr, 0, pkCrc16(hdr, 2, 0x40));
  pkEnc(hdr, 0, 0x40);
  lg.set(hdr, 0);
  lg.set(lst, 0x40);
  return lg;
}

function withMap(fwsc, slots) {
  const lg = new Uint8Array(logicalImage(fwsc));
  const { off, size } = pkOpenController(lg, true);
  writeMap(lg.subarray(off, off + size), slots);
  return resignLogical(lg);
}

function ctlSha(buf) {
  return crypto.createHash('sha256').update(buf).digest('hex');
}

const elite = loadPkg('elite');
const pro = loadPkg('pro');

test('exact 0.0.25.1 Elite package is accepted', () => {
  assert.equal(productOf(elite), cat.versions[0].models.elite.product);
  const ctl = controllerOf(logicalImage(elite));
  assert.equal(ctl.length, 61552);
  assert.equal(ctlSha(ctl), CTL_SHA);
});

test('exact 0.0.25.1 Pro package is accepted', () => {
  assert.equal(productOf(pro), cat.versions[0].models.pro.product);
  const ctl = controllerOf(logicalImage(pro));
  assert.equal(ctl.length, 61552);
  assert.equal(ctlSha(ctl), CTL_SHA);
});

test('off-by-one slot start is rejected', () => {
  for (const start of [0xB2FFF, 0xB3001]) {
    const slots = [[0xC2000, 0x14000], [0xD6000, 0x1E000], [start, 0x8000]];
    const lg = withMap(elite, slots);
    const { flash } = pkOpenController(lg, true);
    assert.equal(pkHasSlotMap(flash), false, 'start ' + start);
    assert.throws(() => controllerOf(lg), /supported SMK slot map/);
  }
});

test('off-by-one slot size is rejected', () => {
  for (const size of [0x7FFF, 0x8001]) {
    const slots = [[0xC2000, 0x14000], [0xD6000, 0x1E000], [0xB3000, size]];
    const lg = withMap(elite, slots);
    const { flash } = pkOpenController(lg, true);
    assert.equal(pkHasSlotMap(flash), false, 'size ' + size);
    assert.throws(() => controllerOf(lg), /supported SMK slot map/);
  }
});

test('overlapping slots are rejected', () => {
  const slots = [[0xC2000, 0x14000], [0xD6000, 0x1E000], [0xD0000, 0x8000]];
  const lg = withMap(elite, slots);
  const { flash } = pkOpenController(lg, true);
  assert.equal(pkHasSlotMap(flash), false);
  assert.throws(() => controllerOf(lg), /supported SMK slot map/);
});

test('a slot past the end is rejected', () => {
  const slots = [[0xC2000, 0x14000], [0xD6000, 0x1E000], [0xF4000, 0x1000]];
  const lg = withMap(elite, slots);
  const { flash } = pkOpenController(lg, true);
  assert.equal(pkHasSlotMap(flash), false);
  assert.throws(() => controllerOf(lg), /supported SMK slot map/);
});

test('a missing slot map is rejected', () => {
  const lg = withMap(elite, null);
  const { flash } = pkOpenController(lg, true);
  assert.equal(pkHasSlotMap(flash), false);
  assert.throws(() => controllerOf(lg), /supported SMK slot map/);
});

test('a wrong model is rejected', () => {
  assert.notEqual(productOf(elite), cat.versions[0].models.pro.product);
  assert.notEqual(productOf(pro), cat.versions[0].models.elite.product);
  const eliteModel = productOf(elite).split('_')[0];
  const proModel = productOf(pro).split('_')[0];
  assert.equal(sameKeyboard(eliteModel, proModel), false);
  assert.equal(sameKeyboard(eliteModel, 'FM-1'), false);
  assert.equal(keyboardModel('FM-1'), null);
});
