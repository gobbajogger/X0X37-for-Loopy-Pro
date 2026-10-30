// SPDX-License-Identifier: GPL-3.0-only
// Existing scan ordering regression from PR #44, adapted for NIGHTLY scan(true) write lock.
// No real Web MIDI, browser permission, USB, firmware or user-data writes.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { Script, createContext } from 'node:vm';

const path = process.argv[2] || new URL('./x0x_installer.html', import.meta.url);
const source = readFileSync(path, 'utf8');
const start = source.indexOf('async function scan(allowPostWrite = false) {');
assert.ok(start >= 0, 'production scan function missing');
const end = source.indexOf('\n}', start);
assert.ok(end > start, 'production scan end missing');
const code = source.slice(start, end + 2);
if (source.includes('<html')) {
  assert.match(source.slice(Math.max(0,start-70),start), /let scanSerial = 0;\s*$/,
    'installer must own the scan generation counter');
  assert.match(source, /writing = true;\s*scanSerial\+\+;/,
    'beginning firmware write must invalidate earlier asynchronous scans');
}
assert.match(code, /async function scan\(allowPostWrite = false\) \{/);

function setup() {
  const state = { connected: false, leds: [], changes: [], selected: [], requests: [], closed: [], lookups: [] };
  const ui = { conn: { textContent: '' } };
  const env = {
    $: id => ui[id] || (ui[id] = {}),
    led: value => state.leds.push(value),
    t: k => k,
    midiAccess: async () => ({}),
    Updater: class {
      find(predicate) {
        return new Promise((resolve, reject) => {
          state.requests.push({ predicate, resolve, reject });
        });
      }
    },
    setConnected: (on, detail) => { state.connected = on; state.changes.push({ on, detail }); },
    suggestModel: model => state.selected.push(model),
  };
  const ctx = createContext({renderConnectionAction(){},refreshJourney(){},...env});
  new Script('let connectionState="idle",writing = false, lastId = null, scanSerial = 0;\n' + code + '\nglobalThis.start=scan; globalThis.setWriting = v => { writing = v; if (v) scanSerial++; }; globalThis.getLastId = () => lastId;').runInContext(ctx);
  const device = (model='SMK-37 Pro') => ({ id: { model, text: model }, name: model + ' MIDI',
    link: { close() { state.closed.push(model); } } });
  const tick = async () => { for (let i=0; i<8; i++) await Promise.resolve(); };
  return { state, ctx, device, tick };
}

async function staleOverwritesWithNotFound() {
  const {state,ctx,device,tick} = setup();
  const first = ctx.start(); await tick(); const second = ctx.start();
  await tick(); assert.equal(state.requests.length, 2);
  state.requests[1].resolve(device()); await second; assert.equal(state.connected,true);
  state.requests[0].resolve(null); await tick();
  // The fixed scan aborts the older request without a redundant loader lookup.
  if (state.requests[2]) state.requests[2].resolve(null);
  await first;
  assert.equal(state.connected,true,'older NOT FOUND scan must not replace newer CONNECTED');
  assert.deepEqual(state.selected,['SMK-37 Pro'],'stale scan must not replace selected model');
  assert.equal(state.requests.length,2,'stale scan should not start redundant OTA scan');
  return 'stale not-found scan cannot replace new connected result';
}
async function staleErrorMustNotOverride() {
  const {state,ctx,device,tick} = setup();
  const first=ctx.start();await tick();const second=ctx.start();
  await tick();
  state.requests[1].resolve(device()); await second;
  state.requests[0].reject(new Error('old unplug error')); await first;
  assert.equal(state.connected,true,'older rejected scan must not overwrite new success');
  assert.equal(state.changes.length,1,'stale error should not modify connection UI');
  return 'stale disconnect error cannot replace new connected result';
}
async function staleWrongModelMustNotOverride() {
  const {state,ctx,device,tick} = setup();
  const first=ctx.start();await tick();const second=ctx.start();
  await tick();
  state.requests[1].resolve(device('SMK-37 Pro')); await second;
  state.requests[0].resolve(device('SMK-37 Elite')); await first;
  assert.deepEqual(state.selected,['SMK-37 Pro'],'older scan must not force wrong model');
  assert.equal(state.closed.length,2,'any stale opened device links must be closed');
  return 'older Elite scan cannot overwrite newer Pro selection';
}
async function writeOverlapsOutstandingScan() {
  const {state,ctx,device,tick} = setup();
  const first=ctx.start();await tick();ctx.setWriting(true);
  state.requests[0].resolve(device());await first;
  assert.equal(state.changes.length,0,'pre-write scan must not alter status after write began');
  assert.equal(state.closed.length,1,'stale discovered device link must close');
  ctx.setWriting(false);
  return 'in-flight scan does not disturb active flashing status';
}
async function latestCanReportNotFoundAndError() {
  for (const mode of ['missing','error','ota']) {
    const {state,ctx,device,tick}=setup();
    const p=ctx.start();await tick();
    if(mode==='error')state.requests[0].reject(new Error('connection denied'));
    else if(mode==='ota') {state.requests[0].resolve(null);await tick();state.requests[1].resolve(device('ota-SMK-37 Elite'));}
    else {state.requests[0].resolve(null);await tick();state.requests[1].resolve(null);}
    await p;
    if(mode==='error')assert.deepEqual(state.changes.map(v=>v.on),[false]);
    if(mode==='missing')assert.deepEqual(state.changes.map(v=>v.on),[false]);
    if(mode==='ota'){assert.equal(state.connected,true);assert.deepEqual(state.selected,['SMK-37 Elite']);}
  }
  return 'latest error, not-found and OTA result still work';
}

async function postwriteScanAllowed() {
  const {state,ctx,device,tick}=setup();
  ctx.setWriting(true);
  const post=ctx.start(true);await tick();
  assert.equal(state.requests.length,1,"post-write scan(true) must run under write lock");
  await ctx.start(); // Blocked user Connect must NOT invalidate post-write rescan.
  state.requests[0].resolve(device());await post;
  assert.equal(state.connected,true);
  assert.equal(state.changes.length,1);
  assert.deepEqual(state.selected, [],
    "postwrite scan must not reload firmware and overwrite saved/restored-slot completion status");
  return 'post-write scan updates connection without clobbering restoration result';
}
async function oldPrewriteCannotReappearAfterWriteEnds() {
  const {state,ctx,device,tick}=setup();
  const first=ctx.start();await tick();
  ctx.setWriting(true);
  ctx.setWriting(false);
  state.requests[0].resolve(device());await first;
  assert.equal(state.changes.length,0,'pre-write scan must stay invalid after write ends');
  assert.equal(state.closed.length,1,'stale pre-write MIDI device must close');
  return 'pre-write scan stays invalid even after permission/write finishes';
}
for (const check of [staleOverwritesWithNotFound,staleErrorMustNotOverride,staleWrongModelMustNotOverride,writeOverlapsOutstandingScan,latestCanReportNotFoundAndError,postwriteScanAllowed,oldPrewriteCannotReappearAfterWriteEnds]) {
  const title=await check();console.log('PASS '+title);
}
console.log('PASS scan-race regression (7 scenarios; NIGHTLY exact source, simulated MIDI)');