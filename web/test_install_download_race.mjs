// SPDX-License-Identifier: GPL-3.0-only
// Regression for the production installer download selection/error race.
// Run: node web/test_install_download_race.mjs (source: web/x0x_installer.html)
// A bare verified excerpt of loadSelectedPackage() is also accepted for offline provenance checks.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { Script, createContext } from 'node:vm';

const path = process.argv[2] || new URL('./x0x_installer.html', import.meta.url);
const source = readFileSync(path, 'utf8');
const anchor = 'async function loadSelectedPackage() {';
const start = source.indexOf(anchor);
assert.ok(start >= 0, 'production installer loadSelectedPackage() absent');
const end = source.indexOf('\n}', start);
assert.ok(end > start, 'production loader function unclosed');
const actualFunction = source.slice(start, end + 2);
assert.match(actualFunction, /requestId !== loadSerial/, 'selection race guards absent');

function deferred() {
  let resolve, reject;
  const promise = new Promise((yes, no) => { resolve = yes; reject = no; });
  return { promise, resolve, reject };
}
const product = 'SMK-37 Pro_900';
const fakePkg = (name) => ({ pkg: `${name}.fwsc`, product, sha256: 'checked' });
const old = { version: 'older', models: { pro: fakePkg('older') } };
const newer = { version: 'newer', models: { pro: fakePkg('newer') } };
const validResponse = () => ({ ok: true, arrayBuffer: async () => new ArrayBuffer(8) });

async function scenario(label, phase) {
  const events = [];
  const dom = { go: { disabled: false }, devagree: { checked: true } };
  const stale = deferred();
  let version = old;
  let newHasImage = false;
  const fake = {
    $: id => dom[id] || (dom[id] = {}),
    currentVersion: () => version,
    isCatalogChecked: () => true,
    refreshMeta() {}, updateInstallGate() {},
    fetch: url => {
      if (url === 'older.fwsc') {
        if (phase === 'body-fails') return Promise.resolve({ ok: true, arrayBuffer: () => stale.promise });
        return stale.promise;
      }
      if (phase === 'current-http-fails') return Promise.resolve({ ok: false, status: 503 });
      return Promise.resolve(validResponse());
    },
    sha256Hex: async () => 'checked',
    productOf: () => product,
    modelKeyForProduct: () => 'pro',
    logicalImage: x => x,
    MODELS: { pro: 'SMK-37 Pro' },
    say: s => { events.push('ready:' + s); newHasImage = true; },
    sayK: (s, detail) => events.push(`${s}${detail ? ':' + detail : ''}`),
  };
  const script = `let loadSerial = 0, chosenModel = 'pro', image = null, selectedProduct = null;\n${actualFunction}\nglobalThis.start = loadSelectedPackage;\nglobalThis.state = () => ({ image: Boolean(image), product: selectedProduct, loadSerial });`;
  const vm = createContext(fake);
  new Script(script).runInContext(vm);
  const previous = vm.start();
  if (phase === 'body-fails') await Promise.resolve();
  version = newer;
  await vm.start();
  const beforeError = events.at(-1);
  if (phase !== 'current-http-fails') assert.equal(newHasImage, true, `${label}: new package did not load`);
  stale.reject(new Error('obsolete firmware download failure'));
  await previous;
  const last = events.at(-1);
  if (phase === 'current-http-fails') {
    assert.equal(beforeError, 'loadfail:newer.fwsc: HTTP 503', `${label}: active error not reported`);
    assert.equal(last, beforeError, `${label}: obsolete error replaced active failure`);
    assert.equal(vm.state().image, false, `${label}: failed download enabled image`);
  } else {
    assert.match(beforeError, /^ready:X0X newer · SMK-37 Pro$/, `${label}: latest selection not ready`);
    assert.equal(last, beforeError, `${label}: obsolete error replaced newer ready message`);
    assert.equal(vm.state().image, true, `${label}: latest firmware image lost`);
    assert.equal(vm.state().product, product, `${label}: latest firmware identity lost`);
  }
  console.log(`PASS ${label}: latest selection remains authoritative`);
}

await scenario('obsolete network failure after newer success', 'fetch-fails');
await scenario('obsolete response body failure after newer success', 'body-fails');
await scenario('obsolete failure cannot replace current HTTP error', 'current-http-fails');
console.log('PASS installer download error ordering (3 mocked cases)');
