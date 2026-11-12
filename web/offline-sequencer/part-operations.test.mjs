// SPDX-License-Identifier: GPL-3.0-only
import test from 'node:test';
import assert from 'node:assert/strict';
import {DraftStore,copy,getPart,bit,validateDraft} from './model.js';
import {partKind,takeFullPart,replaceFullPart,clearFullPart} from './part-operations.js';
test('feature 99 full-part snapshot includes hidden tail, generator and length/rate, but excludes unrelated parts',()=>{
 for(let part=0;part<5;part++){
  const store=new DraftStore();
  store.edit(d=>{const q=getPart(d,0,part);if(part<2){q.length=4;q.rate=1;q.steps[63]={note:83,flags:1,velocity:99};q.generator.density=44;}else if(part<4){q.length=8;q.rate=2;q.hits[0][1]=1;}else{q.gates=1;q.slices[15]=8;q.loopA=12;}});
  const snapshot=takeFullPart(store.doc,0,part);
  const before=copy(store.doc),unrelated=copy(getPart(store.doc,0,(part+1)%5));
  const result=replaceFullPart(store.doc,1,part,snapshot);
  assert.deepEqual(getPart(result,1,part),snapshot.data);
  assert.deepEqual(store.doc,before);
  assert.deepEqual(getPart(result,0,(part+1)%5),unrelated);
  validateDraft(result);
  assert.equal(store.replace(result),true);assert.equal(store.undo(),true);assert.deepEqual(store.doc,before);
 }
});
test('feature 99 clipboard is immutable by subsequent source edits and rejects cross-type or malformed replacements',()=>{
 const store=new DraftStore();const clip=takeFullPart(store.doc,0,0);
 store.edit(d=>{getPart(d,0,0).steps[63]={note:99,flags:1,velocity:110};});
 assert.equal(clip.sourcePattern,0);
 assert.equal(clip.data.steps[63].flags,0);
 assert.throws(()=>replaceFullPart(store.doc,1,2,clip),/does not match/);
 const bad=copy(clip);bad.data.length=999;
 assert.throws(()=>replaceFullPart(store.doc,1,1,bad),/melodic settings/);
});
test('feature 99 clear resets complete stored part but leaves all other contexts alone',()=>{
 const s=new DraftStore();s.edit(d=>{getPart(d,4,0).steps[63].flags=1;getPart(d,4,0).length=9;getPart(d,4,1).steps[3].flags=1;});
 const before=copy(s.doc),candidate=clearFullPart(s.doc,4,0);
 assert.equal(candidate.patterns[4].bass[0].steps[63].flags,0);
 assert.equal(candidate.patterns[4].bass[0].length,16);
 assert.deepEqual(candidate.patterns[4].bass[1],before.patterns[4].bass[1]);
 s.replace(candidate);s.undo();assert.deepEqual(s.doc,before);
});
