// SPDX-License-Identifier: GPL-3.0-only
// Owner-directed source delivery: these tests are written for the box, not run in Codex.
import test from 'node:test';
import assert from 'node:assert/strict';
import {crc32} from '../x0x_breaks.js';
import {openProject} from './project-adapter.js';

// Literal revision-13 ABI fixture, independent of the adapter's offsets/builders.
function fixture() {
  const b=new Uint8Array(36452),v=new DataView(b.buffer);
  [0x50583058,1,36420,0,1,13,13,0].forEach((x,i)=>v.setUint32(i*4,x,true));
  v.setUint32(32,0x50305830,true);v.setUint32(36,1,true);b[50]=2;b[51]=13;
  for(let voice=0;voice<2;voice++) {
    const kind=voice===0?1:0,cell=840+voice*32,tag=2760+voice*12;
    b.set([79,83,2,kind,0,0],cell+26);
    v.setUint32(tag,kind?0x4f4d4931:0x4f434831,true);
    v.setUint16(tag+4,2,true);b[tag+6]=kind?18:22;b[tag+7]=1;
  }
  for(let p=0;p<32;p++) {
    const at=2784+p*656;
    b[at+96]=16;b[at+196]=16;
    for(const off of [200,408]) {
      b[at+off+192]=16;b[at+off+195]=24;b[at+off+199]=1;
    }
  }
  b[50+1]=13;b[36451]=0xad; // Opaque arrangement tail is not normalized.
  v.setUint32(12,crc32(b.subarray(32)),true);return b;
}
function seal(b) {new DataView(b.buffer).setUint32(12,crc32(b.subarray(32)),true);return b;}
const options={model:'SMK-37 Elite'};

test('unchanged revision13 export preserves every byte, including opaque arrangement',()=>{
  const raw=fixture(),project=openProject(raw,options);
  assert.deepEqual(project.draft.engines,['Miami','Chicago']);
  assert.deepEqual(project.exportDraft(project.draft),raw);
});
test('melodic edits use the correct synth, pattern, hidden step and exact release byte',()=>{
  const raw=fixture(),at=2784+31*656+408+63*3;
  raw.set([71,0x7d,0],at);seal(raw);
  const project=openProject(raw,options),draft=project.draft;
  assert.deepEqual(draft.patterns[31].bass[1].steps[63],{note:71,flags:0x7d,velocity:0});
  draft.patterns[31].bass[1].steps[63].note=72;
  const out=project.exportDraft(draft),want=raw.slice();want[at]=72;seal(want);
  assert.deepEqual(out,want);
  assert.equal(raw[at],71);
});
test('drum word32 and generator seed patch without rewriting reserved bytes',()=>{
  const raw=fixture(),project=openProject(raw,options),draft=project.draft;
  draft.patterns[0].drum[1].hits[10][1]=0x80000000;
  draft.patterns[0].bass[0].generator.seed=0x12345678;
  const out=project.exportDraft(draft),want=raw.slice(),v=new DataView(want.buffer);
  v.setUint32(2784+100+10*8+4,0x80000000,true);
  v.setUint32(2784+200+204,0x12345678,true);seal(want);
  assert.deepEqual(out,want);
});
test('BREAK gates, pinned slices and settings retain unrelated pattern swing',()=>{
  const raw=fixture();raw[2784+652]=63;seal(raw);
  const project=openProject(raw,options),draft=project.draft;
  Object.assign(draft.patterns[0].breaks,{gates:0x8001,loopA:2,loopB:4});
  draft.patterns[0].breaks.slices[15]=8;draft.patterns[0].breaks.settings[0]=75;
  const out=project.exportDraft(draft);
  assert.equal(new DataView(out.buffer).getUint32(2784+616,true),0x8001);
  assert.equal(out[2784+620],75);assert.equal(out[2784+632],2);
  assert.equal(out[2784+633],4);assert.equal(out[2784+651],8);
  assert.equal(out[2784+652],63);
});
test('caller mutations cannot rewrite a pinned raw project or baseline draft',()=>{
  const raw=fixture(),project=openProject(raw,options),saved=raw.slice();
  raw.fill(0);const external=project.draft;external.patterns[0].bass[0].steps[0].note=99;
  assert.deepEqual(project.exportDraft(project.draft),saved);
});
test('CRC, exact schema, engine tags and explicit package model fail closed',()=>{
  assert.throws(()=>openProject(fixture(),{model:'Felucca'}),/model/i);
  const corrupt=fixture();corrupt[36451]^=1;assert.throws(()=>openProject(corrupt,options),/CRC/i);
  const old=fixture();new DataView(old.buffer).setUint32(20,12,true);assert.throws(()=>openProject(old,options),/13/);
  const tag=fixture();tag[2760+4]=1;seal(tag);assert.throws(()=>openProject(tag,options),/identity|schema/i);
  const sound=fixture();sound[840+29]=0;seal(sound);assert.throws(()=>openProject(sound,options),/identity/i);
});
test('unrepresentable BREAK intent is refused rather than clamped to eight slices',()=>{
  const raw=fixture();raw[2784+636]=32;seal(raw);
  assert.throws(()=>openProject(raw,options),/BREAK|represent/i);
  assert.equal(raw[2784+636],32);
});
test('unmapped sound, engine, RNG and slot changes cannot silently disappear on export',()=>{
  const project=openProject(fixture(),options);
  const engines=project.draft;engines.engines=['Chicago','Chicago'];
  assert.throws(()=>project.exportDraft(engines),/engine/i);
  const rng=project.draft;rng.patterns[0].bass[0].generator.rng=1;
  assert.throws(()=>project.exportDraft(rng),/RNG/i);
  const slot=project.draft;slot.patterns[0].breaks.loopA=256;
  assert.throws(()=>project.exportDraft(slot),/slot/i);
  const sound=project.draft;sound.sound['mix/0/0']=64;
  assert.throws(()=>project.exportDraft(sound),/sound/i);
});
