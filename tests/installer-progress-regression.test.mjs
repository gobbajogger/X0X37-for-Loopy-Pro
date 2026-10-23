// SPDX-License-Identifier: GPL-3.0-only
// Nightly source regression: permission-stage single-writer guard and honest progress.
// This executes the actual run() function extracted from the current installer source.
import { readFileSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { Script, createContext } from 'node:vm';
import assert from 'node:assert/strict';

const root = fileURLToPath(new URL('../', import.meta.url));
const html = readFileSync(join(root, 'web/x0x_installer.html'), 'utf8');
const start = html.indexOf('async function run(img, product, opts = {}) {');
const stop = html.indexOf('\n}\n\n$("go").addEventListener', start);
assert.ok(start !== -1 && stop > start, 'expected exactly the source installation handler');
const actualRun = html.slice(start, stop + 2);
assert.equal(html.indexOf('async function run(img, product, opts = {}) {', start + 1), -1);

function setup(access) {
  const elements = {
    bar: { value: 100 },
    go: { disabled: false },
    stockgo: { disabled: false },
    stockconfirm: { checked: true },
  };
  const observed = { writes: 0, scans: 0, statuses: [], gates: 0 };
  const env = {
    writing: false, stock: { image: new Uint8Array([1]), product: 'SMK-37 Pro_9000000' },
    lang: 'en', TEXT: { en: {} },
    $: id => elements[id],
    midiAccess: access,
    Updater: class { async find() { return null; } },
    installKeepingSlots: async (_up, _img, _product, _board, hooks) => {
      observed.writes++;
      hooks.step('done');
      return { move: null };
    },
    scan: async () => { observed.scans++; },
    sameKeyboard: () => true,
    log: () => {}, led: () => {}, t: key => key,
    sayK: key => { observed.statuses.push(key); },
    updateInstallGate: () => { observed.gates++; elements.go.disabled = !!env.writing; },
    document: { createElement: () => ({}) }, URL, Blob, setTimeout,
  };
  createContext(env);
  new Script(actualRun + '\nthis.testRun = run;', { filename: 'web/x0x_installer.html' }).runInContext(env);
  const go = () => env.testRun(new Uint8Array([1]), 'SMK-37 Pro_9000000');
  return { env, elements, observed, go };
}

{
  const t = setup(async () => { throw { code: 'denied' }; });
  await t.go();
  assert.equal(t.elements.bar.value, 0, 'permission denial must not retain prior 100%');
  assert.equal(t.env.writing, false, 'permission denial must release exclusive writer');
  assert.deepEqual(t.observed.statuses, ['denied']);
  assert.equal(t.elements.go.disabled, false, 'catalog retry must be restored when otherwise eligible');
  assert.equal(t.elements.stockgo.disabled, false, 'local package retry must be restored when eligible');
  assert.equal(t.observed.writes, 0);
  console.log('PASS denied permission: honest progress and both eligible retry controls restored');
}
{
  const t = setup(async () => ({}));
  t.env.Updater = class { async find() { throw Error('No device'); } };
  await t.go();
  assert.equal(t.elements.bar.value, 0, 'discovery failure must not retain prior 100%');
  assert.equal(t.env.writing, false);
  assert.equal(t.observed.statuses.at(-1), 'error');
  assert.equal(t.observed.writes, 0);
  console.log('PASS discovery error: zero progress, no write, guard released');
}
{
  const t = setup(async () => ({}));
  await t.go();
  assert.equal(t.elements.bar.value, 100, 'a simulated complete operation should reach 100%');
  assert.equal(t.observed.statuses.at(-1), 'done');
  assert.equal(t.observed.writes, 1);
  assert.equal(t.env.writing, false);
  console.log('PASS completion: original done/progress semantics preserved');
}
{
  let allow;
  const permission = new Promise(resolve => { allow = resolve; });
  const t = setup(async () => permission);
  const first = t.go();
  assert.equal(t.env.writing, true, 'writer must be locked while permission is outstanding');
  assert.equal(t.elements.go.disabled, true);
  assert.equal(t.elements.stockgo.disabled, true);
  assert.equal(t.elements.bar.value, 0);
  const second = t.go();
  allow({});
  await Promise.all([first, second]);
  assert.equal(t.observed.writes, 1, 'two presses during permission must not start two writes');
  console.log('PASS concurrent permission: exactly one logical write');
}
{
  let calls = 0;
  const t = setup(async () => { calls++; if (calls === 1) throw { code: 'denied' }; return {}; });
  await t.go();
  assert.equal(t.elements.bar.value, 0);
  await t.go();
  assert.equal(t.elements.bar.value, 100);
  assert.equal(t.observed.writes, 1);
  console.log('PASS denial followed by successful retry without page reload');
}
