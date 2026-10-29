// SPDX-License-Identifier: GPL-3.0-only
// Real archive/session/pre-OTA code with synthetic bytes and in-memory MIDI only.
import test from 'node:test';
import assert from 'node:assert/strict';
import { crc32, userSlotRaw, makeArchive, readArchive, installKeepingSlots } from './x0x_breaks.js';
import { watchDeviceSession, withDeviceOperation } from './device_session.js';
import { parseProject, ProjectMidiLink } from './x0x_projects.js';

function image() {
  const raw=userSlotRaw('ORIGINAL',[{adpcm:Uint8Array.of(1,2,3,4),n:8,rate:22050}],8192);
  const bytes=new Uint8Array(512+raw.data.length).fill(255);bytes.set(raw.hdr);bytes.set(raw.data,512);return bytes;
}
const layout={count:2,size:8192,slots:[{nz:1,size:8192,off:0xc2000},{nz:0,size:8192,off:0xc4000}]};
const archive=makeArchive(layout,[image(),null],'2026-10-30T00:00:00');
function editArchive(edit, bodyEdit=(b)=>b) {
  const oldN=new DataView(archive.buffer).getUint32(12,true), meta=JSON.parse(new TextDecoder().decode(archive.subarray(16,16+oldN)));
  edit(meta);const json=new TextEncoder().encode(JSON.stringify(meta)), body=bodyEdit(archive.slice(16+oldN,-4));
  const out=new Uint8Array(16+json.length+body.length+4);out.set(archive.subarray(0,16));new DataView(out.buffer).setUint32(12,json.length,true);out.set(json,16);out.set(body,16+json.length);new DataView(out.buffer).setUint32(out.length-4,crc32(out.subarray(0,-4)),true);return out;
}
test('complete archive preserves every image byte, including empty slots and reserved header bytes',()=>{
  const parsed=readArchive(archive);assert.deepEqual(parsed.images,[image(),null]);
  assert.throws(()=>makeArchive(layout,[image()],'now'),/incomplete/);
});
for(const [name,edit,body] of [
  ['false empty flag',m=>m.slots[0].empty='false'],
  ['reordered slot',m=>m.slots.reverse()],
  ['missing slot',m=>m.slots.pop()],
  ['aliasing payload',m=>{m.slots[1]={...m.slots[0],slot:2}}],
  ['empty slot with pointers',m=>m.slots[1].offset=0],
  ['fractional offset',m=>m.slots[0].offset=0.5],
  ['trailing bytes',()=>{},b=>Uint8Array.from([...b,0])],
  ['oversize metadata',m=>m.count=0],
]) test('strict browser archive rejects '+name,()=>assert.throws(()=>readArchive(editArchive(edit,body))));

test('disconnect remains invalid after the same port objects reconnect',()=>{
  const access=new EventTarget(),input={id:'i',state:'connected'},output={id:'o',state:'connected'};
  access.inputs=new Map([['i',input]]);access.outputs=new Map([['o',output]]);let invalidations=0;
  const session=watchDeviceSession(access,input,output,()=>invalidations++);
  access.dispatchEvent(new Event('statechange'));session.assert();assert.equal(invalidations,0,'initial port-open event is harmless');
  input.state='disconnected';access.dispatchEvent(new Event('statechange'));input.state='connected';access.dispatchEvent(new Event('statechange'));
  assert.throws(()=>session.assert(),/connection changed/);assert.equal(invalidations,1);session.close();
});
test('closed project link never sends to a reconnected target or accepts its pending response',async()=>{
  let sends=0;const input=new EventTarget(),output={state:'connected',send(){sends++}};
  input.state='connected';const link=new ProjectMidiLink(input,output),pending=link.read(1000);link.close();
  assert.equal(await pending,null);assert.equal(link.send([0xf0,0xf7]),false);assert.equal(sends,0);
});
test('exclusive-origin MIDI owner refuses another workspace without running its callback',async()=>{
  const previous=Object.getOwnPropertyDescriptor(globalThis,'navigator');let calls=0;
  Object.defineProperty(globalThis,'navigator',{configurable:true,value:{locks:{request:async(_name,opts,fn)=>{assert.equal(opts.ifAvailable,true);return fn(null)}}}});
  try { await assert.rejects(withDeviceOperation(()=>calls++),/Another DnBOX workspace/);assert.equal(calls,0); }
  finally { if(previous)Object.defineProperty(globalThis,'navigator',previous);else delete globalThis.navigator; }
});
test('pre-OTA reconnect is refused even when port objects and empty sample bytes are reused',async()=>{
  const access=new EventTarget(),input={id:'in',state:'connected'},output={id:'out',state:'connected'};
  access.inputs=new Map([['in',input]]);access.outputs=new Map([['out',output]]);
  function link(){let response;return {input,output,drain(){},close(){},send(frame){assert.equal(frame[4],15);response=[0xf0,0x7d,0x46,0x4c,15,1,80,0,0,0,0xf7];return true},async read(){const result=response;response=null;return result}}}
  const dev={name:'Keyboard',id:{model:'SMK-37 Elite',text:'SMK-37 Elite_9001300'},link:link()};let writes=0;
  const up={access,find:async()=>dev,async install(_img,product,_step,{beforeUpgrade}){
    input.state='disconnected';access.dispatchEvent(new Event('statechange'));input.state='connected';access.dispatchEvent(new Event('statechange'));
    await beforeUpgrade({...dev,link:link()});writes++;return product;
  }};
  await assert.rejects(installKeepingSlots(up,new Uint8Array(),'SMK-37 Elite_9001300','smk',{}),/connection changed/);assert.equal(writes,0);
});
test('revision 13 uses exact raw project bytes and existing CRC; no migration',()=>{
  const raw=new Uint8Array(36452),v=new DataView(raw.buffer);
  [0x50583058,1,raw.length-32,crc32(raw.subarray(32)),1,13,13,0].forEach((n,k)=>v.setUint32(k*4,n,true));
  assert.equal(parseProject(raw).soundRevision,13);raw[100]^=1;assert.throws(()=>parseProject(raw),/CRC/);
  v.setUint32(20,12,true);assert.throws(()=>parseProject(raw),/incompatible/);
});
