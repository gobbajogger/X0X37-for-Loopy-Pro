// SPDX-License-Identifier: GPL-3.0-only
// Real production installKeepingSlots() extracted from web/x0x_breaks.js,
// with deterministic isolated slot transport fixtures; NEVER contacts a device.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
const source = readFileSync(new URL('./x0x_breaks.js', import.meta.url), 'utf8');
const anchor = 'export async function installKeepingSlots(';
const start = source.indexOf(anchor), end = source.indexOf('\n}', start);
assert.ok(start >= 0 && end > start, 'the real installer transport wrapper must exist');
const body = source.slice(start, end + 2).replace(/^export /, '');
const identicalFamily = (a, b) => a === b ||
  (/^SMK-37 /.test(a) && /^SMK-37 /.test(b));
const images = [Uint8Array.from([0xAA])];
function fixture(deviceModel) {
  const mutations = [], steps = [];
  const dependencies = {
    readSlotLayout: async () => ({ count: 1, slots: [{ nz: true }] }),
    flashMap: () => [{ start: 2, size: 1 }],
    packageSlotMap: () => [{ start: 3, size: 1 }],
    atRisk: () => [0],
    backupSlots: async () => ({ layout: {}, images }),
    archiveStamp: () => ({ name: 'safe-backup.x0xslots', created: '2026-10-23' }),
    makeArchive: () => Uint8Array.from([0x01, 0x02]),
    readArchive: () => true,
    planMigration: () => ({ todo: [0], skipped: [] }),
    restoreMapped: async (link) => { mutations.push(link.model); },
  };
  const installKeepingSlots = new Function(...Object.keys(dependencies),
    body + '\nreturn installKeepingSlots;')(...Object.values(dependencies));
  const up = {
    find: async () => ({ id: { model: 'SMK-37 Pro' }, link: { close() {} } }),
    install: async () => { steps.push('firmware updated'); return 'ok'; },
    waitFor: async (pred) => {
      const id = { model: deviceModel };
      return pred(id) ? { id, link: { model: deviceModel, close() {} } } : null;
    },
  };
  const hooks = { sameKeyboard: identicalFamily, save: () => steps.push('sample backup'),
    say() {}, step() {}, progress() {} };
  return { installKeepingSlots, up, hooks, mutations, steps };
}
async function check(deviceModel, expectedWrites, expectedCode) {
  const t = fixture(deviceModel);
  let code = null;
  try { await t.installKeepingSlots(t.up, Uint8Array.of(0), 'SMK-37 Pro_900', 'smk', t.hooks); }
  catch (e) { code = e.code || e.message; }
  assert.deepEqual(t.mutations, expectedWrites, `restore target must be validated: ${deviceModel}`);
  assert.equal(code, expectedCode, `wrong status after discovering: ${deviceModel}`);
  assert.deepEqual(t.steps, ['sample backup', 'firmware updated'], 'keep backup before firmware install');
  console.log(`PASS ${deviceModel}: restore=${t.mutations.join(',') || 'refused'} status=${code || 'ok'}`);
}
await check('FM-1', [], 'slots'); // A foreign keyboard must never receive backed-up SMK37 samples.
await check('SMK-37 Elite', ['SMK-37 Elite'], null); // Existing approved family equivalence remains.
await check('SMK-37 Pro', ['SMK-37 Pro'], null); // Same model still restores normally.
console.log('PASS wrong-family post-reboot slot-restore guard (3 mocked paths)');
