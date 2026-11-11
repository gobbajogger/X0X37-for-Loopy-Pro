// SPDX-License-Identifier: GPL-3.0-only
// Source-only tests; execution belongs to the Orchestrator's box.
import test from 'node:test';
import assert from 'node:assert/strict';
import {DraftStore} from './model.js';
import {DeviceSync} from './device-sync.js';
import {crc32} from '../x0x_breaks.js';

function projectFixture() {
  const b=new Uint8Array(36452),v=new DataView(b.buffer);
  [0x50583058,1,36420,0,1,13,13,0].forEach((n,i)=>v.setUint32(4*i,n,true));
  v.setUint32(32,0x50305830,true);v.setUint32(36,1,true);b[50]=2;b[51]=13;
  for(let voice=0;voice<2;voice++) {
    const kind=voice===0?1:0,cell=840+32*voice,tag=2760+12*voice;
    b.set([79,83,2,kind,0,0],cell+26);v.setUint32(tag,kind?0x4f4d4931:0x4f434831,true);
    v.setUint16(tag+4,2,true);b[tag+6]=kind?18:22;b[tag+7]=1;
  }
  for(let p=0;p<32;p++) {
    const o=2784+656*p;b[o+96]=16;b[o+196]=16;
    for(const a of [200,408]) {b[o+a+192]=16;b[o+a+195]=24;b[o+a+199]=1;}
  }
  v.setUint32(12,crc32(b.subarray(32)),true);return b;
}

test('current unowned legacy transport sends no read, write or abort commands',async()=>{
  const store=new DraftStore(),sync=new DeviceSync(store),calls=[];
  const device={identity:'X0X current',link:{},projectProtocol:2,
    exportStart(){calls.push('export');},stage(){calls.push('stage');},abort(){calls.push('abort');}};
  sync.attach(device,{model:'SMK-37 Pro'});
  await assert.rejects(sync.read(),/owner|revision|atomic/i);
  await assert.rejects(sync.write(),/owner|revision|atomic/i);
  assert.deepEqual(calls,[]);
  assert.equal(store.revision,0);
});
test('reattachment and disconnect preserve unsent edits and history',()=>{
  const store=new DraftStore(),sync=new DeviceSync(store);
  store.edit(d=>{d.patterns[0].bass[0].steps[0]={note:67,flags:1,velocity:91};});
  const before=structuredClone(store.doc);
  sync.attach({identity:'X0X first',link:{}},{model:'SMK-37 Pro'});
  sync.attach({identity:'X0X second',link:{}},{model:'SMK-37 Elite'});
  sync.disconnect();
  assert.deepEqual(store.doc,before);assert.equal(store.history.length,1);
  assert.equal(store.revision,1);
});
test('USB display names cannot stand in for an identified package model',()=>{
  const sync=new DeviceSync(new DraftStore());
  assert.throws(()=>sync.attach({identity:'X0X keyboard',link:{}},{model:'USB MIDI'}),/model/i);
  assert.throws(()=>sync.attach({identity:'SMK-37 Elite',link:{}},{model:'SMK-37 Elite'}),/identified/i);
});
test('local project export requires an explicit accepted project baseline',()=>{
  const sync=new DeviceSync(new DraftStore());
  assert.throws(()=>sync.exportProject(),/project/i);
});
test('project preview is read-only and a later local edit prevents stale replacement',()=>{
  const store=new DraftStore(),sync=new DeviceSync(store),before=structuredClone(store.doc);
  const preview=sync.preview(projectFixture(),{model:'SMK-37 Elite'});
  assert.deepEqual(store.doc,before);assert.equal(store.revision,0);
  store.edit(d=>{d.patterns[0].bass[0].steps[0]={note:72,flags:1,velocity:80};});
  assert.throws(()=>sync.applyRead(preview),/changed/i);
  assert.equal(store.doc.patterns[0].bass[0].steps[0].note,72);
  assert.equal(store.history.length,1);
});
test('disconnect invalidates pending project previews without discarding local edits',()=>{
  const store=new DraftStore(),sync=new DeviceSync(store);
  const preview=sync.preview(projectFixture(),{model:'SMK-37 Pro'});
  sync.disconnect();assert.throws(()=>sync.applyRead(preview),/changed/i);
  assert.equal(store.revision,0);
});
test('accepted file edits produce a project export and a single replacement Undo',()=>{
  const store=new DraftStore(),sync=new DeviceSync(store),before=structuredClone(store.doc);
  const preview=sync.preview(projectFixture(),{model:'SMK-37 Pro'});
  assert.equal(sync.applyRead(preview),true);assert.equal(store.history.length,1);
  const accepted=sync.exportProject();assert.equal(accepted.length,36452);
  sync.edit(d=>{d.patterns[0].bass[0].steps[63]={note:75,flags:1,velocity:99};});
  const exported=sync.exportProject();assert.deepEqual(Array.from(exported.slice(2784+200+189,2784+200+192)),[75,1,99]);
  store.undo();store.undo();assert.deepEqual(store.doc,before);
  assert.throws(()=>sync.exportProject(),/project|origin/i);
});

test('Undo and Redo restore the raw origin as well as the musical draft',()=>{
  const store=new DraftStore(),sync=new DeviceSync(store);
  const a=projectFixture(),b=projectFixture();
  a[36451]=0xa1;b[36451]=0xb2;
  a.set([60,1,90],2784+200);b.set([62,1,90],2784+200);
  for(const raw of [a,b])new DataView(raw.buffer).setUint32(12,crc32(raw.subarray(32)),true);
  sync.applyRead(sync.preview(a,{model:'SMK-37 Pro'}));
  sync.applyRead(sync.preview(b,{model:'SMK-37 Pro'}));
  store.undo();assert.deepEqual(sync.exportProject(),a);
  store.redo();assert.deepEqual(sync.exportProject(),b);
});
test('unrelated replacements and external edits cannot borrow an old raw origin',()=>{
  const store=new DraftStore(),sync=new DeviceSync(store);
  sync.applyRead(sync.preview(projectFixture(),{model:'SMK-37 Pro'}));
  store.edit(d=>{d.patterns[0].bass[0].steps[0]={note:70,flags:1,velocity:90};});
  assert.throws(()=>sync.exportProject(),/project|origin/i);
  assert.throws(()=>sync.edit(d=>{d.patterns[0].bass[0].length=32;}),/project|origin/i);
  store.undo();assert.equal(sync.exportProject().length,36452);
});
