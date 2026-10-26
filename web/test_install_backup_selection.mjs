// SPDX-License-Identifier: GPL-3.0-only
// Exact-source synthetic backup/approval regression. Run: node web/test_install_backup_selection.mjs
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
const html = readFileSync(new URL('./x0x_installer.html', import.meta.url), 'utf8');
const src = readFileSync(new URL('./x0x_breaks.js', import.meta.url), 'utf8');
const start = src.indexOf('export async function installKeepingSlots(');
const stop = src.indexOf('\n}\n\n// A break pack', start);
assert.ok(start >= 0 && stop > start, 'real helper source present');
const method = src.slice(start, stop + 2);
const guard = '  if (typeof h.beforeFirmwareWrite === "function" && !h.beforeFirmwareWrite())\n' +
  '    throw fail("selectionstale", "Firmware selection changed before writing; nothing was installed.");\n';
assert.ok(method.includes(guard), 'last-safe-point approval check');
assert.match(html, /step, sameKeyboard, beforeFirmwareWrite: selectionValid/);
assert.match(html, /selectionstale: /);
function build(source, state) {
  const deps = {
    readSlotLayout: async () => ({count: 1, slots: [{nz: state.risk ? 1 : 0}]}),
    flashMap: () => [1], packageSlotMap: () => [2], atRisk: () => state.risk ? [0] : [],
    backupSlots: async () => ({layout: {count: 1}, images: [Uint8Array.of(3)]}),
    archiveStamp: () => ({created:'test', name:'backup.x0xslots'}),
    makeArchive: () => Uint8Array.of(7), readArchive: () => true,
    planMigration: () => ({todo:[], skipped:[]}),
    guardMigrationLoopMenu: () => ({todo:[], skipped:[]}),
    restoreMapped: async () => {state.restores++},
  };
  return new Function(...Object.keys(deps), source.replace(/^export /, '') + '\nreturn installKeepingSlots;')(...Object.values(deps));
}
async function trial(source, mode) {
  const state = {risk:mode !== 'no-risk', approved:true, writes:0, restores:0, saves:0, closes:0, release:null};
  const link = {close(){state.closes++}};
  const up = {
    async find(){return {id:{model:'SMK-37 Pro', text:'SMK-37 Pro_900'}, link}},
    async install(){state.writes++; return 'done'},
    async reconnectAfter(){return {id:{model:'SMK-37 Pro'}, link}},
  };
  const pending = build(source, state)(up, Uint8Array.of(1), 'SMK-37 Pro_900', 'smk', {
    sameKeyboard:()=>true, confirmModel:()=>true, beforeFirmwareWrite:()=>state.approved,
    save:async()=>{state.saves++; if(mode==='export-failed')return false;
      if(mode.startsWith('hold-'))await new Promise(resolve=>{state.release=resolve});return true},
  }).then(value=>({value}),error=>({error}));
  await new Promise(resolve=>setImmediate(resolve));
  if(mode==='hold-revoked')state.approved=false;
  if(mode.startsWith('hold-')){assert.equal(typeof state.release,'function');state.release()}
  const outcome=await pending;
  return {...state, outcome};
}
let n=0; const ok=(value,msg)=>{assert.ok(value,msg);n++};
const old = method.replace(guard, '');
ok((await trial(old,'hold-revoked')).writes===1, 'baseline RED: revocation still wrote firmware');
const revoked=await trial(method,'hold-revoked');
ok(revoked.saves===1 && revoked.writes===0, 'revoked after backup: zero firmware writes');
ok(revoked.outcome.error?.code==='selectionstale', 'revoked after backup: clear stale result');
ok(revoked.restores===0 && revoked.closes===1, 'revoked: no restore, port closed');
const kept=await trial(method,'hold-kept');
ok(kept.writes===1 && kept.restores===1 && !kept.outcome.error, 'stable approval: original positive flow');
const failed=await trial(method,'export-failed');
ok(failed.writes===0 && failed.outcome.error?.code==='slotbackup', 'failed export: no write');
const safe=await trial(method,'no-risk');
ok(safe.writes===1 && safe.saves===0, 'no sample risk: unchanged install');
console.log('PASS '+n+' verified synthetic scenarios; no hardware/browser testing');
