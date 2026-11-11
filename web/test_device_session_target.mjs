import test from 'node:test';
import assert from 'node:assert/strict';
import { withDeviceOperation, watchDeviceSession } from './device_session.js';
test('never runs a MIDI writer without exclusive browser-origin lock', async()=>{
  const previous=Object.getOwnPropertyDescriptor(globalThis,'navigator');let ran=0;
  try{
    Object.defineProperty(globalThis,'navigator',{configurable:true,value:{locks:{request:async(name,opts,cb)=>{assert.equal(name,'dnbox-midi-operation');assert.equal(opts.ifAvailable,true);return cb(null)}}}});
    await assert.rejects(withDeviceOperation(async()=>{ran++}),/Another DnBOX workspace/);
    assert.equal(ran,0);
  }finally{if(previous)Object.defineProperty(globalThis,'navigator',previous);else delete globalThis.navigator}
});
test('single owner executes exactly once with granted lock', async()=>{
  const previous=Object.getOwnPropertyDescriptor(globalThis,'navigator');let ran=0;
  try{
    Object.defineProperty(globalThis,'navigator',{configurable:true,value:{locks:{request:async(name,opts,cb)=>cb({name})}}});
    assert.equal(await withDeviceOperation(async()=>++ran),1);
    assert.equal(ran,1);
  }finally{if(previous)Object.defineProperty(globalThis,'navigator',previous);else delete globalThis.navigator}
});
test('missing Web Locks declines device operation (not an unsafe fallback)', async()=>{
  const previous=Object.getOwnPropertyDescriptor(globalThis,'navigator');let ran=0;
  try{
    Object.defineProperty(globalThis,'navigator',{configurable:true,value:{}});
    await assert.rejects(withDeviceOperation(()=>ran++),/Exclusive browser MIDI access is unavailable/);
    assert.equal(ran,0);
  }finally{if(previous)Object.defineProperty(globalThis,'navigator',previous);else delete globalThis.navigator}
});
test('same-id stale port object cannot be reused', ()=>{
  const access=new EventTarget(),input={id:'input',state:'connected'},output={id:'output',state:'connected'};
  access.inputs=new Map([[input.id,input]]);access.outputs=new Map([[output.id,output]]);let n=0;
  const session=watchDeviceSession(access,input,output,()=>n++);
  access.dispatchEvent(new Event('statechange'));session.assert();assert.equal(n,0);
  access.inputs.set(input.id,{id:'input',state:'connected'});
  access.dispatchEvent(new Event('statechange'));
  assert.throws(()=>session.assert(),/connection changed/);
  assert.equal(n,1);
  session.close();
});
test('disconnect/reconnect cannot resurrect dead session',()=>{
  const access=new EventTarget(),input={id:'i',state:'connected'},output={id:'o',state:'connected'};
  access.inputs=new Map([['i',input]]);access.outputs=new Map([['o',output]]);let n=0;
  const session=watchDeviceSession(access,input,output,()=>n++);
  input.state='disconnected';access.dispatchEvent(new Event('statechange'));
  input.state='connected';access.dispatchEvent(new Event('statechange'));
  assert.throws(()=>session.assert(),/connection changed/);assert.equal(n,1);
  session.close();
});
