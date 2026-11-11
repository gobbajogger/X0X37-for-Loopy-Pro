// SPDX-License-Identifier: GPL-3.0-only
import test from 'node:test';
import assert from 'node:assert/strict';
import {DraftStore,copy,getPart} from './model.js';
import {applyPlan} from './editing.js';
import {selectNotes,takeNotes,groupPlan} from './note-groups.js';

function fixture() {
 const s=new DraftStore();
 s.edit(d=>{
  const b=getPart(d,0,0);b.length=4;
  b.steps[1]={note:60,flags:13,velocity:0};
  b.steps[2]={note:61,flags:70,velocity:37};
  b.steps[3]={note:59,flags:146,velocity:19};
  b.steps[8]={note:67,flags:1,velocity:111};
  b.steps[63]={note:70,flags:129,velocity:12};
 });
 return s;
}

test('feature 98 marquee intersects exact note duration and includes hidden-tail notes',()=>{
 const b=getPart(fixture().doc,0,0);
 assert.deepEqual(selectNotes(b,{start:2,end:3,low:60,high:60}),[1]);
 // Step 3 has a 9/16 release, so the first note ends at 3 + 9/16.
 assert.deepEqual(selectNotes(b,{start:3.5625,end:4,low:0,high:127}),[]);
 assert.deepEqual(selectNotes(b,{start:63,end:64,low:70,high:70}),[63]);
 assert.deepEqual(selectNotes(b,{start:0,end:64,low:61,high:66}),[]);
 assert.deepEqual(selectNotes(b,{start:8,end:8,low:0,high:127}),[]);
 assert.throws(()=>selectNotes(b,{start:0,end:65,low:0,high:127}),/rectangle/);
});

test('feature 98 clipboard snapshots complete raw cells without aliasing later edits',()=>{
 const s=fixture(),before=copy(s.doc),clip=takeNotes(s.doc,0,0,[8,1]);
 assert.equal(clip.schema,'dnbox-note-group/1');
 assert.equal(clip.kind,'melodic');
 assert.deepEqual(clip.notes,[
  {start:0,cells:[{note:60,flags:13,velocity:0},{note:61,flags:70,velocity:37},{note:59,flags:146,velocity:19}]},
  {start:7,cells:[{note:67,flags:1,velocity:111}]}
 ]);
 assert.deepEqual(s.doc,before);
 s.edit(d=>{getPart(d,0,0).steps[2].velocity=99;});
 assert.equal(clip.notes[0].cells[1].velocity,37);
});

test('feature 98 move preserves complex tie metadata and advances as one Undo frame',()=>{
 const s=fixture(),before=copy(s.doc),history=s.history.length;
 const plan=groupPlan(s,0,0,[1,8],'move',{steps:2,semitones:3});
 assert.deepEqual(s.doc,before);
 assert.deepEqual(plan.selection,[3,10]);
 assert.deepEqual(getPart(plan.candidate,0,0).steps.slice(3,6),[
  {note:63,flags:13,velocity:0},{note:64,flags:70,velocity:37},{note:62,flags:146,velocity:19}
 ]);
 assert.deepEqual(getPart(plan.candidate,0,0).steps[10],{note:70,flags:1,velocity:111});
 assert.deepEqual(getPart(plan.candidate,0,0).steps[63],before.patterns[0].bass[0].steps[63]);
 assert.equal(applyPlan(s,plan),true);
 assert.equal(s.history.length,history+1);
 assert.equal(s.undo(),true);assert.deepEqual(s.doc,before);
 assert.equal(s.redo(),true);assert.deepEqual(s.doc,plan.candidate);
});

test('feature 98 group move clears every selected source before checking destinations',()=>{
 const s=new DraftStore();
 s.edit(d=>{getPart(d,0,0).steps[2]={note:60,flags:1,velocity:30};getPart(d,0,0).steps[4]={note:62,flags:1,velocity:40};});
 const plan=groupPlan(s,0,0,[2,4],'move',{steps:2,semitones:0});
 assert.deepEqual(plan.selection,[4,6]);
 assert.deepEqual(getPart(plan.candidate,0,0).steps[4],{note:60,flags:1,velocity:30});
 assert.deepEqual(getPart(plan.candidate,0,0).steps[6],{note:62,flags:1,velocity:40});
});

test('feature 98 collision in any member rejects the whole group without a history frame',()=>{
 const s=fixture(),before=copy(s.doc),history=s.history.length,revision=s.revision;
 assert.throws(()=>groupPlan(s,0,0,[1],'move',{steps:5,semitones:0}),/overlap/);
 assert.deepEqual(s.doc,before);assert.equal(s.history.length,history);assert.equal(s.revision,revision);
});

test('feature 98 move bounds reject wrapping and transposition overflow in any raw tie cell',()=>{
 const s=fixture(),before=copy(s.doc);
 for(const [starts,options] of [
  [[63],{steps:1,semitones:0}],[[1],{steps:-2,semitones:0}],
  [[1],{steps:0,semitones:67}],[[1],{steps:0,semitones:-60}],
  [[1],{steps:0.5,semitones:0}]
 ])assert.throws(()=>groupPlan(s,0,0,starts,'move',options),/bounds|integer/);
 assert.deepEqual(s.doc,before);
});

test('feature 98 delete and cut preserve other parts and the unselected hidden tail',()=>{
 const s=fixture(),before=copy(s.doc),clip=takeNotes(s.doc,0,0,[1,8]);
 const plan=groupPlan(s,0,0,[1,8],'delete');
 assert.deepEqual(plan.selection,[]);
 const b=getPart(plan.candidate,0,0);
 assert.deepEqual(b.steps.slice(1,4),[
  {note:60,flags:0,velocity:0},{note:61,flags:0,velocity:37},{note:59,flags:0,velocity:19}
 ]);
 assert.equal(b.steps[8].flags,0);assert.deepEqual(b.steps[63],before.patterns[0].bass[0].steps[63]);
 assert.deepEqual(plan.candidate.patterns[0].bass[1],before.patterns[0].bass[1]);
 applyPlan(s,plan);
 const pasted=groupPlan(s,1,1,[],'paste',{clipboard:clip,start:5});
 assert.deepEqual(pasted.selection,[5,12]);
 assert.deepEqual(getPart(pasted.candidate,1,1).steps.slice(5,8),clip.notes[0].cells);
});

test('feature 99 group velocity Set updates onsets only and retains raw TIE values and endpoints',()=>{
 const s=fixture(),before=copy(s.doc),plan=groupPlan(s,0,0,[1,8],'velocity',{value:127});
 const b=getPart(plan.candidate,0,0);
 assert.deepEqual(b.steps[1],{note:60,flags:13,velocity:127});
 assert.deepEqual(b.steps[8],{note:67,flags:1,velocity:127});
 assert.deepEqual(b.steps.slice(2,4),before.patterns[0].bass[0].steps.slice(2,4));
 for(const value of [0,128,1.5,NaN])assert.throws(()=>groupPlan(s,0,0,[1],'velocity',{value}),/velocity/);
 assert.deepEqual(s.doc,before);
});

test('feature 98 paste is an independent snapshot and rejects occupied cells and hidden-tail overflow',()=>{
 const s=fixture(),clip=takeNotes(s.doc,0,0,[1,8]),before=copy(s.doc);
 const plan=groupPlan(s,1,1,[],'paste',{clipboard:clip,start:0,pitchOffset:-12});
 assert.deepEqual(getPart(plan.candidate,1,1).steps.slice(0,3),[
  {note:48,flags:13,velocity:0},{note:49,flags:70,velocity:37},{note:47,flags:146,velocity:19}
 ]);
 assert.deepEqual(plan.selection,[0,7]);
 assert.throws(()=>groupPlan(s,0,0,[],'paste',{clipboard:clip,start:1}),/overlap/);
 assert.throws(()=>groupPlan(s,1,1,[],'paste',{clipboard:clip,start:58}),/bounds/);
 assert.deepEqual(s.doc,before);
 assert.deepEqual(clip.notes[0].cells[0],{note:60,flags:13,velocity:0});
});

test('feature 98 no operation can select an orphan TIE or a stale/missing onset',()=>{
 const s=fixture();
 s.edit(d=>{getPart(d,0,0).steps[20]={note:50,flags:2,velocity:9};});
 for(const starts of [[2],[20],[7],[],[1,1],[64],[1.5],Array(1)]){
  assert.throws(()=>takeNotes(s.doc,0,0,starts),/selection|onset/);
  assert.throws(()=>groupPlan(s,0,0,starts,'delete'),/selection|onset/);
 }
 assert.throws(()=>takeNotes(s.doc,0,2,[1]),/melodic/);
 assert.throws(()=>groupPlan(s,32,0,[1],'delete'),/target/);
});

test('feature 98 moving or pasting before an orphan TIE cannot silently absorb it',()=>{
 const s=fixture(),clip=takeNotes(s.doc,0,0,[8]);
 s.edit(d=>{getPart(d,0,0).steps[20]={note:50,flags:2,velocity:9};});
 const before=copy(s.doc);
 assert.throws(()=>groupPlan(s,0,0,[8],'move',{steps:11,semitones:0}),/overlap|TIE/);
 assert.throws(()=>groupPlan(s,0,0,[],'paste',{clipboard:clip,start:19}),/overlap|TIE/);
 assert.deepEqual(s.doc,before);
});

test('feature 98 clipboard validation rejects malformed, overlapping or noncanonical note groups',()=>{
 const s=fixture(),good=takeNotes(s.doc,0,0,[1,8]),before=copy(s.doc);
 const cases=[
  c=>c.schema='other',c=>c.kind='drum',c=>c.notes=[],
  c=>c.notes[0].start=1,c=>c.notes[1].start=2,
  c=>c.notes[0].cells[0].flags=2,c=>c.notes[0].cells[1].flags=1,
  c=>c.notes[0].cells[0].velocity=128,c=>c.notes[0].cells[0].extra=true,
  c=>c.notes[0].cells[1].note=-1,c=>c.notes[0].cells.length=65,
  c=>c.sourcePart=2,c=>c.sourcePattern=32
 ];
 for(const mutate of cases){const bad=copy(good);mutate(bad);assert.throws(()=>groupPlan(s,1,0,[],'paste',{clipboard:bad,start:0}),/clipboard|cells|bounds/);}
 assert.deepEqual(s.doc,before);
});

test('feature 98 applying a stale group preview cannot replace newer local edits',()=>{
 const s=fixture(),plan=groupPlan(s,0,0,[1],'delete');
 s.edit(d=>{getPart(d,0,0).steps[8].velocity=55;});
 const before=copy(s.doc);
 assert.throws(()=>applyPlan(s,plan),/changed after preview/);
 assert.deepEqual(s.doc,before);
});

test('feature 98 no-op move neither adds an Undo frame nor destroys Redo',()=>{
 const s=fixture();s.edit(d=>{getPart(d,0,0).steps[8].velocity=55;});s.undo();
 const history=s.history.length,future=s.future.length,revision=s.revision;
 assert.equal(applyPlan(s,groupPlan(s,0,0,[1,8],'move',{steps:0,semitones:0})),false);
 assert.equal(s.history.length,history);assert.equal(s.future.length,future);assert.equal(s.revision,revision);
});

test('feature 98 unknown commands or misspelled options cannot silently create another edit',()=>{
 const s=fixture(),before=copy(s.doc);
 assert.throws(()=>groupPlan(s,0,0,[1],'unknown'),/Unsupported/);
 assert.throws(()=>groupPlan(s,0,0,[1],'move',{steps:1,semitones:0,pitch:8}),/options/);
 assert.throws(()=>groupPlan(s,0,0,[1],'velocity',{value:100,semitones:12}),/options/);
 assert.deepEqual(s.doc,before);
});
