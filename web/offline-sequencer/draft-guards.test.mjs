// SPDX-License-Identifier: GPL-3.0-only
import test from 'node:test';
import assert from 'node:assert/strict';
import {SafeDraftStorage} from './draft-guards.js';
const key='dnbox-offline-draft-v2',fallback=key+':last-good';
const good=n=>JSON.stringify({schema:'dnbox-sequencer-draft/2',note:n});
function validator(raw){
  if(typeof raw!=='string'||raw.length>1024)throw Error('oversize');
  const d=JSON.parse(raw);
  if(!d||d.schema!=='dnbox-sequencer-draft/2'||!Number.isInteger(d.note)||d.note<0||d.note>127||Object.keys(d).length!==2)throw Error('invalid');
  return d;
}
function validate(d){if(!d||d.schema!=='dnbox-sequencer-draft/2'||!Number.isInteger(d.note)||d.note<0||d.note>127||Object.keys(d).length!==2)throw Error('invalid');}
class FakeLocks {
  constructor(){this.tail=Promise.resolve();this.names=[];this.active=0;this.maxActive=0;}
  request(name,opts,fn){
    assert.equal(opts.mode,'exclusive');this.names.push(name);
    const next=this.tail.then(async()=>{
      this.active++;this.maxActive=Math.max(this.maxActive,this.active);
      try{return await fn({name,mode:'exclusive'});}finally{this.active--;}
    });
    this.tail=next.catch(()=>{});
    return next;
  }
}
function storage(initial={},fault=null){
  const map=new Map(Object.entries(initial)),written=[];
  return {map,written,getItem(k){if(fault?.read===k)throw Error('unreadable');return map.has(k)?map.get(k):null;},setItem(k,v){if(fault?.write===k)throw Error('write failed');written.push([k,v]);map.set(k,v);}};
}
const draft=n=>({schema:'dnbox-sequencer-draft/2',note:n});
const create=(s,locks)=>new SafeDraftStorage(s,{parseDraft:validator,validateDraft:validate,locks});
test('corrupt sole primary cannot be overwritten by an edit, even with a lock',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:'{broken'}),d=create(s,locks),r=d.load();
  assert.equal(r.status,'locked');assert.equal(r.draft,null);
  await assert.rejects(d.save(draft(80)),/locked/);
  assert.equal(s.getItem(key),'{broken');assert.deepEqual(s.written,[]);
});
test('loading valid primary does not write backup outside exclusive lock',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:good(60)}),d=create(s,locks);
  assert.equal(d.load().status,'ready');assert.deepEqual(s.written,[]);
  await d.save(draft(61));
  assert.deepEqual(s.written,[[fallback,good(60)],[key,good(61)]]);
  assert.equal(locks.maxActive,1);
});
test('explicit backup restore is serialized and retains backup on success',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:'corrupt',[fallback]:good(65)}),d=create(s,locks);
  assert.equal(d.load().status,'recovery');assert.deepEqual(s.written,[]);
  await assert.rejects(d.save(draft(90)),/locked/);
  assert.equal((await d.restore()).note,65);
  assert.equal(s.getItem(key),good(65));assert.equal(s.getItem(fallback),good(65));
  await d.save(draft(66));assert.equal(s.getItem(key),good(66));assert.equal(s.getItem(fallback),good(65));
});
test('new draft first writes verified fallback, then primary while locked',async()=>{
  const locks=new FakeLocks(),s=storage(),d=create(s,locks);
  assert.equal(d.load().status,'new');await d.save(draft(42));
  assert.deepEqual(s.written,[[fallback,good(42)],[key,good(42)]]);
});
test('invalid document cannot change saved primary or fallback',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:good(45),[fallback]:good(44)}),d=create(s,locks);
  d.load();await assert.rejects(d.save(draft(200)),/invalid/);
  assert.equal(s.getItem(key),good(45));assert.equal(s.getItem(fallback),good(44));assert.deepEqual(s.written,[]);
});
test('failed primary write retains valid recovery copy; failed backup write leaves both intact',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:good(45),[fallback]:good(44)},{write:key}),d=create(s,locks);
  d.load();await assert.rejects(d.save(draft(46)),/write failed/);
  assert.equal(s.getItem(key),good(45));assert.equal(s.getItem(fallback),good(45));
  const x=storage({[key]:good(45),[fallback]:good(44)},{write:fallback}),y=create(x,locks);
  y.load();await assert.rejects(y.save(draft(46)),/write failed/);
  assert.equal(x.getItem(key),good(45));assert.equal(x.getItem(fallback),good(44));
});
test('two overlapping tabs serialize cross-tab writes: one succeeds and the stale one fails',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:good(60),[fallback]:good(59)});
  const a=create(s,locks),b=create(s,locks);
  a.load();b.load();
  const [first,second]=await Promise.allSettled([a.save(draft(61)),b.save(draft(62))]);
  assert.equal(first.status,'fulfilled');assert.equal(second.status,'rejected');
  assert.match(String(second.reason),/Another tab/);
  assert.equal(s.getItem(key),good(61));assert.equal(s.getItem(fallback),good(60));
  assert.equal(locks.maxActive,1);
  assert.equal(locks.names.length,2);
});
test('two tabs cannot interleave restore and edit either',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:'broken',[fallback]:good(40)}),a=create(s,locks),b=create(s,locks);
  a.load();b.load();
  const [one,two]=await Promise.allSettled([a.restore(),b.restore()]);
  assert.equal(one.status,'fulfilled');assert.equal(two.status,'rejected');
  assert.equal(s.getItem(key),good(40));assert.equal(s.getItem(fallback),good(40));
});
test('Web Locks unavailable: reject every write without damaging existing data',async()=>{
  const s=storage({[key]:good(60),[fallback]:good(59)}),d=create(s,null);
  d.load();await assert.rejects(d.save(draft(61)),/HTTPS or localhost/);
  assert.deepEqual(s.written,[]);
  assert.equal(s.getItem(key),good(60));
  const r=storage({[key]:'corrupt',[fallback]:good(50)}),c=create(r,null);
  c.load();await assert.rejects(c.restore(),/HTTPS or localhost/);
  assert.equal(r.getItem(key),'corrupt');assert.equal(r.getItem(fallback),good(50));
});
test('storage read failure blocks all saves',async()=>{
  const s=storage({[key]:good(60)},{read:key}),d=create(s,new FakeLocks());
  assert.equal(d.load().status,'locked');await assert.rejects(d.save(draft(70)),/locked/);
  assert.deepEqual(s.written,[]);
});
test('first primary write interrupted leaves verified backup recoverable after restart',async()=>{
  const locks=new FakeLocks(),fault={write:key},s=storage({},fault),a=create(s,locks);
  a.load();await assert.rejects(a.save(draft(63)),/write failed/);
  assert.equal(s.getItem(key),null);assert.equal(s.getItem(fallback),good(63));
  fault.write=null;
  const b=create(s,locks);assert.equal(b.load().status,'recovery');assert.equal((await b.restore()).note,63);
});
test('corrupt backup remains untouched on load and is replaced only inside a locked save',async()=>{
  const s=storage({[key]:good(70),[fallback]:'{corrupt'}),d=create(s,new FakeLocks());
  assert.equal(d.load().status,'ready');assert.deepEqual(s.written,[]);
  await d.save(draft(71));
  assert.equal(s.getItem(fallback),good(70));assert.equal(s.getItem(key),good(71));
});
test('sequential stale-tab save also fails without a hidden overwrite',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:good(9),[fallback]:good(8)});
  const a=create(s,locks),b=create(s,locks);a.load();b.load();
  await a.save(draft(10));
  await assert.rejects(b.save(draft(11)),/Another tab/);
  assert.equal(s.getItem(key),good(10));assert.equal(s.getItem(fallback),good(9));
});
test('same-tab overlapping saves preserve commit order and last-good rollback',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:good(21),[fallback]:good(20)}),d=create(s,locks);
  assert.equal(d.load().status,'ready');
  const [first,second]=await Promise.allSettled([d.save(draft(22)),d.save(draft(23))]);
  assert.equal(first.status,'fulfilled');assert.equal(second.status,'fulfilled');
  assert.equal(s.getItem(key),good(23));assert.equal(s.getItem(fallback),good(22));
  assert.equal(locks.maxActive,1);
});
test('external corruption after load rejects without rotating backup into corruption',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:good(41),[fallback]:good(40)}),d=create(s,locks);
  assert.equal(d.load().status,'ready');
  s.setItem(key,'{invalid');const before=s.written.length;
  await assert.rejects(d.save(draft(42)),/Another tab/);
  assert.equal(s.written.length,before);
  assert.equal(s.getItem(key),'{invalid');assert.equal(s.getItem(fallback),good(40));
});
test('repeated contested saves never silently confirm more than one stale writer',async()=>{
  const locks=new FakeLocks(),s=storage({[key]:good(30),[fallback]:good(29)});
  for(let round=0;round<12;round++){
    const first=create(s,locks),second=create(s,locks),third=create(s,locks);
    first.load();second.load();third.load();
    const stateBefore=s.getItem(key);
    const attempts=await Promise.allSettled([
      first.save(draft(40+round)),second.save(draft(70+round)),third.save(draft(100+round))
    ]);
    assert.equal(attempts.filter(x=>x.status==='fulfilled').length,1);
    assert.equal(attempts.filter(x=>x.status==='rejected').length,2);
    assert.equal(s.getItem(key),good(40+round));
    assert.equal(s.getItem(fallback),stateBefore);
  }
  assert.equal(locks.maxActive,1);
});
