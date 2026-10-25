// SPDX-License-Identifier: GPL-3.0-only
// Registered host regression for the actual source method. Synthetic slot responses;
// no Web MIDI hardware and no firmware write. Run from the repository root:
// node web/test_installer_backup_contract.mjs
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import assert from 'node:assert/strict';

const dir = dirname(fileURLToPath(import.meta.url));
const source = readFileSync(join(dir, 'x0x_breaks.js'), 'utf8');
const marker = 'export async function installKeepingSlots(';
assert.equal(source.split(marker).length - 1, 1, 'exactly one shipping helper');
const fnSource = source.slice(source.indexOf(marker));
let checks = 0;
const check = (c, reason) => { assert.ok(c, reason); checks++; };
const deferred = () => { let resolve, reject;
  const promise = new Promise((yes, no) => { resolve = yes; reject = no; });
  return { promise, resolve, reject }; };

function scenario({ risk = true, save = () => true, guard = () => true, family = true } = {}) {
  const e = { writes: 0, restores: 0, closes: 0, saved: 0 };
  const dependencies = {
    readSlotLayout: async () => ({ count: 1, slots: [{ nz: 1 }] }),
    flashMap: () => [[1]], packageSlotMap: () => [[2]],
    atRisk: () => risk ? [0] : [],
    backupSlots: async () => ({ layout: { count: 1 }, images: [Uint8Array.of(3)] }),
    archiveStamp: () => ({ created: '2026-10-25', name: 'owner-backup.x0xslots' }),
    makeArchive: () => Uint8Array.from([7, 9, 11]),
    readArchive: () => true,
    planMigration: () => { e.restores++; throw Error('unsafe discovery'); },
    restoreMapped: async () => { e.restores++; throw Error('unsafe restore'); },
  };
  const method = new Function(...Object.keys(dependencies),
    fnSource.replace(/^export\s+/, '') + '\nreturn installKeepingSlots;')(...Object.values(dependencies));
  const link = { close() { e.closes++; } };
  const device = { id: { model: 'SMK-37 Pro', text: 'SMK-37 Pro_9001100' }, link };
  const updater = {
    async find() { return device; },
    async install() { e.writes++; return 'SMK-37 Pro_9001100'; },
    async waitFor() { e.restores++; throw Error('unsafe MIDI reselect'); },
  };
  const hooks = { sameKeyboard: () => family, prewriteGuard: guard,
    say() {}, progress() {}, step() {}, confirmModel: () => true,
    save: (bytes, name) => { e.saved++; return save(bytes, name); } };
  const run = async () => {
    try { return { result: await method(updater, Uint8Array.of(1),
      'SMK-37 Pro_9001100', 'smk', hooks), error: null }; }
    catch (error) { return { result: null, error }; }
  };
  return { e, run };
}
for (const [reason, save] of [
  ['explicit false backup export', () => false],
  ['rejected async backup export', () => Promise.reject(Error('disk rejected'))],
]) {
  const a = scenario({ save });
  const result = await a.run();
  check(result.error?.code === 'slotbackup', reason + ' must reject');
  check(result.error?.detail === 'owner-backup.x0xslots', reason + ' preserves backup filename');
  check(a.e.writes === 0 && a.e.restores === 0, reason + ' writes nothing');
  check(a.e.closes === 1, reason + ' closes original MIDI link');
}
{
  const waiting = deferred();
  const a = scenario({ save: () => waiting.promise });
  const pending = a.run();
  await new Promise(resolve => setImmediate(resolve));
  check(a.e.saved === 1, 'async backup export requested');
  check(a.e.writes === 0, 'pending owner backup blocks first firmware write');
  waiting.resolve(true);
  const { result, error } = await pending;
  check(!error && result?.move?.manualRestoreRequired === true, 'verified backup proceeds with manual restoration');
  check(a.e.writes === 1 && a.e.restores === 0, 'no same-name physical-device restoration lookup');
  check(result?.move?.skipped?.[0] === 1, 'owner is warned about original slot');
}
{
  let approved = true;
  const a = scenario({ save: async () => { approved = false; return true; }, guard: () => approved });
  const { error } = await a.run();
  check(error?.code === 'selectionstale', 'approval revoked while saving is rejected');
  check(a.e.writes === 0 && a.e.restores === 0, 'revocation prevents OTA');
}
{
  const a = scenario({ risk: false, save: () => { throw Error('no backup needed'); } });
  const { result, error } = await a.run();
  check(!error && result?.move === null, 'no at-risk samples use existing install path');
  check(a.e.writes === 1 && a.e.saved === 0, 'no unnecessary backup prompt');
}
{
  const a = scenario({ family: false });
  const { error } = await a.run();
  check(error?.code === 'model' && a.e.writes === 0, 'wrong-family keyboard remains refused');
}
console.log(`installer backup/approval/manual-restore host regression: PASS ${checks}/${checks}`);
