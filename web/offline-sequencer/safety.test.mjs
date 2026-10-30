// SPDX-License-Identifier: GPL-3.0-only
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {DraftStore,createDraft,copy,getPart,parseDraft,validateDraft} from './model.js';
import {putNote,notesOf} from './editing.js';
import {parseSMF,makeImportPlan} from './smf.js';
import {StorageRevision,SelectionEpoch,SafeDraftStorage} from './draft-guards.js';
const vlq=n=>{const a=[n&127];while(n>>>=7)a.unshift((n&127)|128);return a;};
function midi(ticks){const events=[0,144,60,96,...vlq(ticks),128,60,0,0,255,47,0];return new Uint8Array([77,84,104,100,0,0,0,6,0,0,0,1,1,224,77,84,114,107,0,0,0,events.length,...events]);}
const map={pattern:0,part:0,track:0,channel:0,rate:0,startBeat:1,beats:1};
test('feature 93: exact grid and half-grid MIDI releases stay lossless',()=>{
 for(const [ticks,end] of [[60,.5],[120,1],[180,1.5],[240,2],[300,2.5],[360,3],[420,3.5],[480,4]]){
  const plan=makeImportPlan(parseSMF(midi(ticks)),createDraft(),0,map);
  assert.deepEqual(plan.errors,[],String(ticks));
  assert.equal(notesOf(getPart(plan.candidate,0,0))[0].end,end,String(ticks));
  assert.ok(!plan.losses.some(x=>x.kind==='release rounded'),String(ticks));
 }
});
test('feature 100: Undo/Redo bounded at 32, new edit clears redo',()=>{
 const s=new DraftStore(),base=copy(s.doc);
 for(let i=0;i<36;i++)s.edit(d=>{d.patterns[0].bass[0].length=17+i;});
 assert.equal(s.history.length,32);assert.equal(s.future.length,0);
 const current=copy(s.doc);
 assert.equal(s.undo(),true);assert.equal(s.future.length,1);
 assert.equal(s.redo(),true);assert.deepEqual(s.doc,current);assert.equal(s.future.length,0);
 assert.equal(s.undo(),true);s.edit(d=>putNote(d,0,0,{start:10,pitch:65}));
 assert.equal(s.redo(),false);assert.notDeepEqual(s.doc,base);
});
test('feature 95: stale tab never overwrites another saved draft',()=>{
 const state=new Map(),key='dnbox-offline-draft-v2';
 const storage={getItem:k=>state.has(k)?state.get(k):null,setItem:(k,v)=>state.set(k,v)};
 const a=new StorageRevision(storage.getItem(key)),b=new StorageRevision(storage.getItem(key));
 assert.equal(a.save(storage,key,{pitch:61}),true);assert.equal(b.changed(storage.getItem(key)),true);
 assert.equal(b.save(storage,key,{pitch:62}),false);
 assert.deepEqual(JSON.parse(storage.getItem(key)),{pitch:61});
});
test('features 93/95: file selection epochs guard stale asynchronous reads',async()=>{
 const picker=new SelectionEpoch(),first=picker.next(),second=picker.next();
 assert.equal(picker.valid(first),false);assert.equal(picker.valid(second),true);
 const source=await readFile(new URL('./editor.js',import.meta.url),'utf8');
 const midi=source.slice(source.indexOf("$('#midi-file').onchange="),source.indexOf('function showImportSettings'));
 const read=midi.indexOf('await f.arrayBuffer()'),check=midi.indexOf('captured.epoch!==uiEpoch',read),parse=midi.indexOf("workerRequest('parse'");
 assert.ok(read>=0&&check>read&&check<parse,'stale read checked before shared worker parse');
 const open=source.slice(source.indexOf("$('#open-draft').onchange="),source.indexOf('function soundKey'));
 const fileRead=open.indexOf('await f.text()'),tokenCheck=open.indexOf('draftPick.valid(token)',fileRead),modal=open.indexOf("modal('Replace complete");
 assert.ok(fileRead>=0&&tokenCheck>fileRead&&tokenCheck<modal,'draft picker token checked before confirmation');
});

test('real draft schema: corruption never replaces last good saved copy',async()=>{
 const key='dnbox-offline-draft-v2',backup=key+':last-good';
 const values=new Map(),writes=[];
 const storage={
  getItem:k=>values.has(k)?values.get(k):null,
  setItem(k,v){writes.push(k);values.set(k,v);}
 };
 const fresh=createDraft(),first=JSON.stringify(fresh);
 values.set(key,first);
 const locks={tail:Promise.resolve(),request(_name,opts,fn){
  assert.equal(opts.mode,'exclusive');
  const next=this.tail.then(()=>fn());this.tail=next.catch(()=>{});return next;
 }};
 const session=new SafeDraftStorage(storage,{parseDraft,validateDraft,locks});
 const loaded=session.load();
 assert.equal(loaded.status,'ready');
 assert.deepEqual(loaded.draft,fresh);
 assert.equal(values.get(key),first);
 assert.equal(values.has(backup),false,'loading does not write storage outside a lock');
 const bad=copy(fresh);bad.patterns[0].bass[0].length=65;
 const writesBefore=writes.length;
 await assert.rejects(session.save(bad),/Invalid melodic settings/);
 assert.equal(writes.length,writesBefore);
 assert.equal(values.get(key),first);
 const updated=copy(fresh);updated.patterns[0].bass[0].length=17;
 assert.equal(await session.save(updated),true);
 assert.deepEqual(parseDraft(values.get(key)),updated);
 assert.equal(values.get(backup),first);
 values.set(key,'{broken');
 const recovered=new SafeDraftStorage(storage,{parseDraft,validateDraft,locks});
 const countBefore=writes.length;
 assert.equal(recovered.load().status,'recovery');
 assert.equal(writes.length,countBefore,'loading corrupt data must not mutate storage');
 await assert.rejects(recovered.save(fresh),/locked/);
 assert.equal(values.get(key),'{broken');
 assert.deepEqual(await recovered.restore(),fresh);
 assert.equal(values.get(backup),first,'explicit recovery retains last good copy');
});
