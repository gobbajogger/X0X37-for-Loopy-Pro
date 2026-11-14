// Exact-source browser input routing without a WASM binary or simulated sound.
// The handler block is read from production web/main.js; only its conditional
// dynamic module import is injected with the REAL heldRouter implementation.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { heldRouter } from './controls.js';

const source = readFileSync(new URL('../main.js', import.meta.url), 'utf8');
const first = source.indexOf('const NKEY = 37, BTN0 = 37;');
const last = source.indexOf('// ---------------------------------------------------------------- MIDI in', first);
assert(first >= 0 && last > first, 'could not find the production input event handlers');
const snippet = source.slice(first, last);
assert.match(snippet, /await import\('\.\/controls\.js'\)/);
const injected = snippet.replace("await import('./controls.js')", 'Promise.resolve({heldRouter})');
const AsyncFunction = Object.getPrototypeOf(async function () {}).constructor;

async function mount() {
  const events = [], listeners = {}, canvasEvents = {}, docEvents = [];
  const analog = new Uint16Array(23), down = new Uint8Array(64);
  analog[20] = 535; analog[21] = 629;
  let hit = 112, booting = 0, running = {};
  const canvas = {
    getBoundingClientRect() { return { left: 0, top: 0, width: 100, height: 100 }; },
    setPointerCapture() {},
    addEventListener(type, fn) { canvasEvents[type] = fn; }
  };
  const document = {
    addEventListener(type, fn) { (listeners['doc:' + type] ||= []).push(fn); }
  };
  const addEventListener = (type, fn) => { (listeners[type] ||= []).push(fn); };
  const ui = {
    panel_hit() { return hit; },
    panel_hit_y() { return .75; },
    panel_slide() { return 512; },
    panel_enc_id(id) { return id; }
  };
  const setKey = (id, on) => {
    if (down[id] === Number(on)) return;
    down[id] = Number(on);
    events.push({ type: 'key', id, down: !!on });
  };
  const setAnalog = (id, value) => {
    if (analog[id] === value) return;
    analog[id] = value;
    events.push({ type: 'analog', id, value });
  };
  const env = {
    DNBOX: true, board: 1, W: 100, H: 100, canvas, document, addEventListener,
    ui, heldRouter, down, analog, setKey, setAnalog, setVelocity() {}, turn() {}, setPot() {},
    PITCH_REST: 535, MOD_REST: 629, booting, running,
    HTMLSelectElement: class {}, HTMLInputElement: class {},
    HTMLTextAreaElement: class {}, HTMLButtonElement: class {}
  };
  const factory = new AsyncFunction(...Object.keys(env),
    injected + '\nreturn {press,unpress,pressPad,unpressPad,releaseDnboxInputs,dnboxHolds,keyboardDown,pointers};');
  const api = await factory(...Object.values(env));
  const emit = (target, kind, props = {}) => {
    const e = { clientX: 50, clientY: 50, pointerId: 5, key: 'a', code: 'KeyA',
      target: {}, repeat: false, preventDefault() {}, ...props };
    if (target === 'canvas') {
      assert(canvasEvents[kind], 'missing canvas event ' + kind);
      canvasEvents[kind](e);
    } else if (target === 'document') {
      const handlers = listeners['doc:' + kind] || [];
      assert(handlers.length, 'missing document event ' + kind);
      handlers.forEach(fn => fn(e));
    } else {
      const handlers = listeners[kind] || [];
      assert(handlers.length, 'missing global event ' + kind);
      handlers.forEach(fn => fn(e));
    }
  };
  return {
    ...api, events, emit, down, analog,
    hit(value) { hit = value; },
    hardware() { return events.filter(x => x.type === 'key'); },
    dispatch(detail) { emit('document', 'dnbox:input', { detail }); },
    canvasDown(pointerId, key = 12) { hit = 100 + key; emit('canvas', 'pointerdown', { pointerId }); },
    canvasUp(pointerId, type = 'pointerup') { emit('canvas', type, { pointerId }); },
    keyboardDownKey(key = 'a', code = 'KeyA', target = {}) {
      emit('window', 'keydown', { key, code, target });
    },
    keyboardUpKey(key = 'a', code = 'KeyA', target = {}) {
      emit('window', 'keyup', { key, code, target });
    }
  };
}

const onOff = id => [{ type: 'key', id, down: true }, { type: 'key', id, down: false }];

test('canvas C4, then desktop C4: keyboard releases first without silencing touch', async () => {
  const x = await mount();
  x.canvasDown(10, 12);
  x.keyboardDownKey();
  x.keyboardUpKey();
  assert.deepEqual(x.hardware(), [onOff(12)[0]]);
  assert.equal(x.dnboxHolds.countFor('key', 12), 1);
  x.canvasUp(10);
  assert.deepEqual(x.hardware(), onOff(12));
});
test('desktop C4, then canvas C4: pointer releases first without silencing keyboard', async () => {
  const x = await mount();
  x.keyboardDownKey();
  x.canvasDown(10, 12);
  x.canvasUp(10);
  assert.deepEqual(x.hardware(), [onOff(12)[0]]);
  x.keyboardUpKey();
  assert.deepEqual(x.hardware(), onOff(12));
});
test('accessible held key joins keyboard and canvas, all release orders preserve last owner', async () => {
  for (const order of [['access','keyboard','canvas'], ['canvas','access','keyboard']]) {
    const x = await mount();
    x.canvasDown(6, 12);
    x.keyboardDownKey();
    x.dispatch({kind: 'key', id: 12, down: true});
    const release = {
      canvas: () => x.canvasUp(6),
      keyboard: () => x.keyboardUpKey(),
      access: () => x.dispatch({kind: 'key', id: 12, down: false})
    };
    for (const source of order.slice(0, 2)) {
      release[source]();
      assert.deepEqual(x.hardware(), [onOff(12)[0]], source + ' released too soon');
    }
    release[order[2]]();
    assert.deepEqual(x.hardware(), onOff(12));
  }
});
test('two canvas pointers on the same note and glissando preserve independent ownership', async () => {
  const x = await mount();
  x.canvasDown(101, 12);
  x.canvasDown(202, 12);
  x.canvasUp(101);
  assert.deepEqual(x.hardware(), [onOff(12)[0]]);
  x.hit(115); // next key after C4
  x.emit('canvas', 'pointermove', {pointerId: 202});
  assert.deepEqual(x.hardware(), [onOff(12)[0], onOff(12)[1], {type:'key',id:15,down:true}]);
  x.canvasUp(202, 'pointercancel');
  x.canvasUp(202, 'lostpointercapture'); // idempotent
  assert.deepEqual(x.hardware(), [...onOff(12), ...onOff(15)]);
});
test('keyboard repeat, focus transfer and blur cannot leak or steal notes', async () => {
  const x = await mount();
  x.canvasDown(7, 12);
  x.keyboardDownKey();
  x.emit('window','keydown',{key:'a',code:'KeyA',repeat:true});
  x.emit('window','keydown',{key:'a',code:'KeyA',repeat:false});
  assert.equal(x.dnboxHolds.countFor('key', 12), 2);
  x.keyboardUpKey('a','KeyA', new (class {})());
  assert.deepEqual(x.hardware(), [onOff(12)[0]]);
  x.emit('window','blur');
  assert.deepEqual(x.hardware(), onOff(12));
  x.canvasUp(7, 'pointercancel');
  x.keyboardUpKey();
  assert.deepEqual(x.hardware(), onOff(12));
  assert.equal(x.dnboxHolds.count, 0);
});
test('boot/reset release, cancellation and pad ownership are idempotent', async () => {
  const x = await mount();
  x.canvasDown(10, 12);
  x.keyboardDownKey();
  x.dispatch({kind:'key',id:12,down:true});
  x.releaseDnboxInputs(); // exactly what the production boot reset calls
  assert.deepEqual(x.hardware(), onOff(12));
  assert.equal(x.keyboardDown.size, 0);
  assert.equal(x.pointers.size, 0);
  x.canvasUp(10, 'pointercancel');
  x.keyboardUpKey();
  x.dispatch({kind:'key',id:12,down:false});
  assert.deepEqual(x.hardware(), onOff(12));

  x.canvasDown(8, 12);
  x.canvasUp(8, 'lostpointercapture');
  assert.deepEqual(x.hardware(), [...onOff(12), ...onOff(12)]);

  x.hit(200); // pad 0 via canvas
  x.emit('canvas', 'pointerdown', {pointerId:51});
  x.dispatch({kind:'pad',id:0,down:true});
  x.keyboardDownKey('1','Digit1');
  x.canvasUp(51);
  x.dispatch({kind:'pad',id:0,down:false});
  assert.deepEqual(x.events.filter(e=>e.type==='analog'&&e.id===0), [{type:'analog',id:0,value:700}]);
  x.keyboardUpKey('1','Digit1');
  assert.deepEqual(x.events.filter(e=>e.type==='analog'&&e.id===0),
    [{type:'analog',id:0,value:700},{type:'analog',id:0,value:0}]);
});

test('production boot and tab-hide source call ownership cleanup, and MIDI is routed through it', () => {
  const full = source;
  assert.match(full, /document\.dispatchEvent\(new Event\('dnbox:booting'\)\);\s*releaseDnboxInputs\(\)/);
  assert.match(full, /document\.visibilityState === "hidden"[\s\S]{0,80}releaseDnboxInputs\(\)/);
  assert.match(full, /'midi:'[\s\S]{0,160}press\(k, token\)/);
});
