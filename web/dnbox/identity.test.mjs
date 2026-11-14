import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { audioPolicy, PINNED, runtimeCapabilities, validateCatalog } from './identity.mjs';

const catalog = JSON.parse(readFileSync(new URL('./firmwares.json', import.meta.url)));
const copy = () => structuredClone(catalog);
test('catalog includes only real 0.0.25.1 packages with distinct model-proven hashes', () => {
  assert.equal(validateCatalog(catalog), catalog);
  assert.equal(new Set(catalog.firmwares.map(f => f.sha256)).size, 2);
  assert.equal(Object.keys(PINNED).length, 2);
  assert(catalog.firmwares.every(f => f.package.startsWith('https://gobbajogger.github.io/')));
});
test('Elite package cannot be presented as Pro', () => {
  const invalid = copy();
  invalid.firmwares[1].sha256 = invalid.firmwares[0].sha256;
  assert.throws(() => validateCatalog(invalid), /model, identity|provenance/);
});
test('model, source, version and URL tampering fail before runtime import', () => {
  for (const [key, bad] of [
    ['model', 'SMK-37 Elite_900'],
    ['version', '0.0.26'],
    ['source_commit', 'deadbeef'],
    ['size', 651720],
    ['package', 'https://evil.example/payload.fwsc'],
  ]) {
    const invalid = copy();
    invalid.firmwares[1][key] = bad;
    assert.throws(() => validateCatalog(invalid), undefined, key);
  }
  const repeated = copy();
  repeated.firmwares[1].id = repeated.firmwares[0].id;
  assert.throws(() => validateCatalog(repeated));
});
test('cannot insert an unreviewed version or remove a model', () => {
  const extra = copy();
  extra.firmwares.push({ ...extra.firmwares[0], id: 'dnbox-026' });
  assert.throws(() => validateCatalog(extra));
  const one = copy();
  one.firmwares.pop();
  assert.throws(() => validateCatalog(one));
});
test('real-clock and low-latency defaults are explicit; buffered 2.5s is rejected', () => {
  const p = audioPolicy(new URLSearchParams());
  assert.deepEqual([p.mhz,p.ahead,p.latency], [360,512,'interactive']);
  assert(p.queueMs > 11 && p.queueMs < 12);
  for (const val of ['-1','0','128','500','110250','2500','NaN']) {
    assert.throws(() => audioPolicy(new URLSearchParams('ahead='+val)), undefined, val);
  }
  assert.equal(audioPolicy(new URLSearchParams('ahead=256')).ahead, 256);
  assert.equal(audioPolicy(new URLSearchParams('ahead=1024')).ahead, 1024);
  assert.throws(() => audioPolicy(new URLSearchParams('mhz=999')));
});
test('missing audio worklet and insecure origin produce honest unsupported state', () => {
  const ok = { isSecureContext:true, WebAssembly:{instantiate(){}}, AudioContext:function(){},
    AudioWorkletNode:function(){}, ResizeObserver:function(){}, crypto:{subtle:{digest(){}}} };
  assert.deepEqual(runtimeCapabilities(ok), { ok:true, missing:[] });
  const fail = runtimeCapabilities({ ...ok, AudioWorkletNode:undefined, isSecureContext:false });
  assert.equal(fail.ok, false);
  assert(fail.missing.includes('AudioWorklet'));
  assert(fail.missing.includes('HTTPS or localhost'));
});
