// SPDX-License-Identifier: GPL-3.0-only
import test from 'node:test';
import assert from 'node:assert/strict';
import {SafeDraftStorage} from './draft-guards.js';
const key='dnbox-offline-draft-v2',fallback=key+':last-good';
const good = n => JSON.stringify({schema:'dnbox-sequencer-draft/2',note:n});
function validator(raw) {
  if(typeof raw!=='string'||raw.length>1024)throw Error('oversize');
  const obj=JSON.parse(raw);
  if(!obj||obj.schema!=='dnbox-sequencer-draft/2'||!Number.isInteger(obj.note)||obj.note<0||obj.note>127||Object.keys(obj).length!==2)throw Error('invalid');
  return obj;
}
function validate(d){if(!d||!Number.isInteger(d.note)||d.note<0||d.note>127||d.schema!=='dnbox-sequencer-draft/2'||Object.keys(d).length!==2)throw Error('invalid');}
function storage(initial={},fault=null){
  const values=new Map(Object.entries(initial)),written=[];
  return {values,written,getItem(k){if(fault?.read===k)throw Error('unreadable');return values.has(k)?values.get(k):null;},setItem(k,v){if(fault?.write===k)throw Error('write failed');written.push([k,v]);values.set(k,v);}};
}
const create=(s)=>new SafeDraftStorage(s,{parseDraft:validator,validateDraft:validate});
test('corrupt only saved draft is never replaced, even after a fresh in-memory edit',()=>{
  const s=storage({[key]:'{missing'}),d=create(s);const r=d.load();
  assert.equal(r.status,'locked');assert.equal(r.draft,null);
  assert.throws(()=>d.save({schema:'dnbox-sequencer-draft/2',note:60}),/locked/);
  assert.equal(s.getItem(key),'{missing');assert.deepEqual(s.written,[]);
});
test('validated primary seeds safety copy without changing primary',()=>{
  const s=storage({[key]:good(42)}),d=create(s);
  assert.equal(d.load().draft.note,42);
  assert.equal(s.getItem(key),good(42));assert.equal(s.getItem(fallback),good(42));
  assert.deepEqual(s.written,[[fallback,good(42)]]);
});
test('valid backup allows explicit recovery but rejects autosave before confirmation',()=>{
  const s=storage({[key]:'corrupted',[fallback]:good(72)}),d=create(s);
  const r=d.load();assert.equal(r.status,'recovery');assert.equal(r.draft.note,72);
  assert.deepEqual(s.written,[]);assert.throws(()=>d.save({schema:'dnbox-sequencer-draft/2',note:80}),/locked/);
  assert.equal(s.getItem(key),'corrupted');
  assert.equal(d.restore().note,72);assert.equal(s.getItem(key),good(72));assert.equal(s.getItem(fallback),good(72));
  assert.equal(d.save({schema:'dnbox-sequencer-draft/2',note:80}),true);
  assert.equal(s.getItem(key),good(80));assert.equal(s.getItem(fallback),good(72));
});
test('fresh valid save writes validated backup before primary',()=>{
  const s=storage(),d=create(s);assert.equal(d.load().status,'new');
  d.save({schema:'dnbox-sequencer-draft/2',note:5});
  assert.deepEqual(s.written,[[fallback,good(5)],[key,good(5)]]);
  d.save({schema:'dnbox-sequencer-draft/2',note:6});
  assert.equal(s.getItem(key),good(6));assert.equal(s.getItem(fallback),good(5));
});
test('invalid candidate cannot overwrite primary or fallback',()=>{
  const s=storage({[key]:good(12),[fallback]:good(11)}),d=create(s);d.load();
  assert.throws(()=>d.save({schema:'dnbox-sequencer-draft/2',note:130}),/invalid/);
  assert.equal(s.getItem(key),good(12));assert.equal(s.getItem(fallback),good(11));
  assert.deepEqual(s.written,[]);
});
test('write failure preserves an intact saved copy',()=>{
  const s=storage({[key]:good(45),[fallback]:good(44)},{write:key}),d=create(s);d.load();
  assert.throws(()=>d.save({schema:'dnbox-sequencer-draft/2',note:46}),/write failed/);
  assert.equal(s.getItem(key),good(45));assert.equal(s.getItem(fallback),good(45));
  const t=storage({[key]:good(45),[fallback]:good(44)},{write:fallback}),u=create(t);u.load();
  assert.throws(()=>u.save({schema:'dnbox-sequencer-draft/2',note:46}),/write failed/);
  assert.equal(t.getItem(key),good(45));assert.equal(t.getItem(fallback),good(44));
});
test('stale tab fails before any writes and does not touch newer saved draft',()=>{
  const s=storage({[key]:good(60),[fallback]:good(59)}),a=create(s),b=create(s);a.load();b.load();
  a.save({schema:'dnbox-sequencer-draft/2',note:61});
  assert.throws(()=>b.save({schema:'dnbox-sequencer-draft/2',note:62}),/Another tab/);
  assert.equal(s.getItem(key),good(61));assert.equal(s.getItem(fallback),good(60));
});
test('storage read failure blocks all writes, even if a backup might exist',()=>{
  const s=storage({[key]:good(60),[fallback]:good(59)},{read:key}),d=create(s);assert.equal(d.load().status,'locked');
  assert.throws(()=>d.save({schema:'dnbox-sequencer-draft/2',note:61}),/locked/);
  assert.deepEqual(s.written,[]);
});
test('corrupted backup is seeded from verified primary, never the reverse',()=>{
  const s=storage({[key]:good(70),[fallback]:'garbage'}),d=create(s);
  assert.equal(d.load().status,'ready');assert.equal(s.getItem(fallback),good(70));
});
test('failed first primary write leaves recovery copy usable on next load',()=>{
  const s=storage({}, {write:key}),a=create(s);a.load();
  assert.throws(()=>a.save({schema:'dnbox-sequencer-draft/2',note:65}),/write failed/);
  assert.equal(s.getItem(key),null);assert.equal(s.getItem(fallback),good(65));
  const b=create(s);assert.equal(b.load().status,'recovery');assert.equal(b.load().draft.note,65);
});
