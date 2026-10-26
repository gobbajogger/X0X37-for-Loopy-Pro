// SPDX-License-Identifier: GPL-3.0-only
// Actual private production Link + public Updater; synthetic MIDI, no devices.
import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {runInNewContext} from 'node:vm';
import {Updater,pack7} from '../web/fm1ota.js';
import {ProjectDevice} from '../web/x0x_projects.js';
const source=readFileSync(new URL('../web/fm1ota.js',import.meta.url),'utf8');
const start=source.indexOf('class Link {'),end=source.indexOf('\nasync function handshake(',start);
const Link=runInNewContext(source.slice(start,end)+'\nLink',{Array,Promise,setTimeout,clearTimeout});
function make({events=false,previous=null,send=()=>{}}={}) {
 const handlers=new Set();
 const input={state:'connected',onmidimessage:previous};
 if(events){input.addEventListener=(_n,f)=>handlers.add(f);input.removeEventListener=(_n,f)=>handlers.delete(f);}
 const output={state:'connected',send};const link=new Link(input,output);
 const emit=data=>{if(events){for(const f of [...handlers])f({data});}else input.onmidimessage?.({data});};
 return {link,input,output,emit,handlers};
}
const packet=n=>Uint8Array.from({length:n},(_,i)=>i===0?0xf0:i===n-1?0xf7:i%127);
const rejected=p=>assert.rejects(p,e=>e.code==='queueoverflow');
function request(addr=0,len=8){const b=[0,0x59,0x30,8,0,0,0,addr&255,(addr>>>8)&255,(addr>>>16)&255,(addr>>>24)&255,len&255,len>>>8,0];return[0xf0,...pack7([...b,~b.slice(6).reduce((a,b)=>a+b,0)&255]),0xf7];}

test('all known reply maxima survive64-packet legitimate FIFO bursts',async()=>{
 for(const n of [20,41,42,43,64,265,266,267]){
  const{link,emit}=make();for(let i=0;i<64;i++)emit(packet(n));
  assert.equal(link.q.length,64);assert.equal(link.queuedBytes,n*64);
  for(let i=0;i<64;i++)assert.deepEqual(await link.read(20),Array.from(packet(n)));
  assert.equal(link.queuedBytes,0);link.close();
 }
});
test('byte budget fails closed before count budget; never silently trims',async()=>{
 const{link,emit}=make();for(let i=0;i<32;i++)emit(packet(1024));
 assert.equal(link.q.length,32);assert.equal(link.queuedBytes,32768);
 emit(packet(4));assert.equal(link.q.length,0);assert.equal(link.queuedBytes,0);
 await rejected(link.read(20));assert.throws(()=>link.send([1]),e=>e.code==='queueoverflow');
 link.drain();await rejected(link.read(20));link.close();
});
test('frame size checked before allocation, even with a pending reader',async()=>{
 for(const n of [1025,Number.MAX_SAFE_INTEGER,NaN,Infinity,-1,1]){
  const{link,emit}=make();const wait=rejected(link.read(100));
  const data={0:0xf0,length:n,[Symbol.iterator](){throw new Error('must not copy oversize frame');}};
  emit(data);await wait;assert.equal(link.queuedBytes,0);link.close();
 }
});
test('maximum admitted frame and malformed short payload consume a bounded queue',async()=>{
 const{link,emit}=make();emit(packet(1024));assert.equal((await link.read(20)).length,1024);
 emit([0x90,64,127]);assert.equal(link.q.length,0);
 emit([0xf0,1]);assert.deepEqual(await link.read(20),[0xf0,1]);link.close();
});
test('FIFO multiple inflight reads retain every reply with separate timeouts',async()=>{
 const{link,emit}=make();const a=link.read(10),b=link.read(150);
 assert.equal(await a,null);const c=link.read(150);emit([0xf0,2,0xf7]);emit([0xf0,3,0xf7]);
 assert.deepEqual(await b,[0xf0,2,0xf7]);assert.deepEqual(await c,[0xf0,3,0xf7]);
 assert.equal(link.readers.length,0);link.close();
});
test('drain retires old reader/timer generation without cancelling a new reader',async()=>{
 const{link,emit}=make();const old=link.read(5);link.drain();assert.equal(await old,null);
 const fresh=link.read(100);await new Promise(r=>setTimeout(r,15));emit([0xf0,9,0xf7]);
 assert.deepEqual(await fresh,[0xf0,9,0xf7]);link.close();
});
test('reader-budget overflow rejects all pending readers explicitly',async()=>{
 const{link}=make();const waits=[];for(let i=0;i<65;i++)waits.push(rejected(link.read(100)));
 await Promise.all(waits);assert.equal(link.readers.length,0);assert.equal(link.q.length,0);link.close();
});
test('close cancels pending wait, clears packet bytes and preserves handler ownership',async()=>{
 for(const events of [false,true]){
  let prior=0;const previous=()=>prior++;const{link,input,emit,handlers}=make({events,previous});
  const pending=link.read(100);link.close();assert.equal(await pending,null);assert.equal(link.readers.length,0);
  assert.equal(input.onmidimessage,previous);assert.equal(handlers.size,0);
  emit(packet(4));assert.equal(link.q.length,0);if(!events)assert.equal(prior,1);
 }
});
test('closing one event listener does not detach another live link or app handler',async()=>{
 const{link,input,output,emit,handlers}=make({events:true});const second=new Link(input,output);
 link.close();assert.equal(handlers.size,1);emit(packet(267));assert.equal((await second.read(20)).length,267);second.close();
 const x=make();const newer=()=>{};x.input.onmidimessage=newer;x.link.close();assert.equal(x.input.onmidimessage,newer);
});
test('actual serve rejects flood after pending reply resolves, before any chunk is sent',async()=>{
 const{link,emit,input}=make();let sent=0;link.output.send=()=>sent++;
 const serving=new Updater({inputs:new Map(),outputs:new Map()}).serve(link,new Uint8Array(16),0xf0000000,100);
 const done=rejected(serving);emit(request());for(let i=0;i<65;i++)emit(packet(4));await done;
 assert.equal(sent,0);assert.equal(link.closed,true);assert.equal(input.onmidimessage,null);
});
test('actual serve cannot report finish success after overflow during finish send',async()=>{
 const{link,emit}=make();link.output.send=()=>{for(let i=0;i<65;i++)emit(packet(4));};
 emit(request(0xf0000000,0));await rejected(new Updater({inputs:new Map(),outputs:new Map()}).serve(link,new Uint8Array(16),0xf0000000,100));
 assert.equal(link.closed,true);
});
test('legitimate retries and finish ack use actual Link and serve without dropping replies',async()=>{
 let sent=0;const{link,emit}=make({send:()=>sent++});
 emit(request());emit(request());emit(request(0xf0000000,0));
 const result=await new Updater({inputs:new Map(),outputs:new Map()}).serve(link,new Uint8Array(16),0xf0000000,100);
 assert.equal(result.finished,true);assert.equal(result.served,2);assert.equal(sent,3);link.close();
});

test('actual ProjectDevice preserves max-size reply and stale-offset correlation through Link',async()=>{
 const {link,emit}=make();const dev=new ProjectDevice(link);dev.identity='X0X test';
 const reply=off=>[0xf0,0x7d,0x46,0x4c,9,66,0,off&127,(off>>7)&127,0,...new Array(256).fill(0),0xf7];
 link.output.send=()=>{emit(reply(0));emit(reply(224));};
 const r=await dev.command(66,[96,1,0,96,1],{length:261,offset:224});
 assert.equal(r.length,261);assert.equal(r[2]+r[3]*128,224);assert.equal(link.queuedBytes,0);link.close();
});

test('handshake overflow fails explicitly and closes its own MIDI listener',async()=>{
 let writes=0;const input={name:'SMK-37 Elite',state:'connected',onmidimessage:null,async open(){}};
 const output={name:input.name,state:'connected',async open(){},send(){writes++;const handler=input.onmidimessage;for(let i=0;i<16000;i++)handler({data:packet(4)});}};
 const up=new Updater({inputs:new Map([['i',input]]),outputs:new Map([['o',output]])});
 await rejected(up.find());assert.equal(writes,1);assert.equal(input.onmidimessage,null);
});
test('allocation follows validated length rather than an untrusted iterable',async()=>{
 const {link,emit}=make();emit({0:0xf0,1:0xf7,length:2,[Symbol.iterator](){throw new Error('must not iterate external data');}});
 assert.deepEqual(await link.read(20),[0xf0,0xf7]);link.close();
});

test('actual Link finish-output throw reports loss, never completion',async()=>{
 const {link,emit}=make({send:()=>{throw new Error('USB output lost');}});
 emit(request(0xf0000000,0));
 const result=await new Updater({inputs:new Map(),outputs:new Map()}).serve(link,new Uint8Array(16),0xf0000000,100);
 assert.equal(result.finished,false);assert.equal(result.lost,true);link.close();
});
test('failed loader finish acknowledgment prevents actual resume success and progress done',async()=>{
 const up=new Updater({inputs:new Map([['i',{name:'SMK-37 Elite',state:'connected'}]]),outputs:new Map()});let closed=0,sends=0;const steps=[];
 const packets=[request(0xf0000000,0)];
 const link={lost:false,async read(){return packets.shift()||null;},send(){return ++sends===1;},close(){closed++;}};
 const dev={link,id:{text:'ota-SMK-37 Elite_900',model:'ota-SMK-37 Elite',version:900},name:'SMK-37 Elite'};
 up.find=async()=>dev;
 if(typeof up.matchingPorts==='function')up.matchingPorts=async()=>[];
 const result=up.resume(new Uint8Array(16),(step)=>steps.push(step),'SMK-37 Elite_900');
 assert.equal(await result,false);assert.equal(sends,2,'upgrade accepted, finish acknowledgment refused');assert.ok(closed>0);assert.ok(!steps.includes('done'));
});

test('resolved pending or queued replies cannot escape a later drain generation',async()=>{
 for(const queued of [false,true]){
  const {link,emit}=make();if(queued)emit(packet(20));const old=link.read(100);if(!queued)emit(packet(20));
  link.drain();assert.equal(await old,null);const fresh=link.read(100);emit(packet(41));assert.equal((await fresh).length,41);link.close();
 }
});
test('terminal overflow rejects an already resolved reply before read returns it',async()=>{
 for(const queued of [false,true]){
  const{link,emit}=make();if(queued)emit(packet(20));const pending=rejected(link.read(100));if(!queued)emit(packet(20));
  for(let i=0;i<65;i++)emit(packet(4));await pending;assert.equal(link.q.length,0);assert.equal(link.closed,true);
 }
});

test('actual handshake preserves identity behind63 unrelated queued packets',async()=>{
 const input={name:'SMK-37 Elite',state:'connected',onmidimessage:null,async open(){}};
 const payload=[...new TextEncoder().encode('SMK-37 Elite_900')];while(payload.length<27)payload.push(0);
 const identity=[0xf0,...pack7([0,0x59,0x11,27,0,0,...payload,~payload.reduce((a,b)=>a+b,0)&255]),0xf7];
 const output={name:input.name,state:'connected',async open(){},send(){for(let i=0;i<63;i++)input.onmidimessage({data:packet(4)});input.onmidimessage({data:identity});}};
 const up=new Updater({inputs:new Map([['i',input]]),outputs:new Map([['o',output]])});
 const dev=await up.find();assert.equal(dev.id.text,'SMK-37 Elite_900');assert.equal(dev.link.queuedBytes,0);dev.link.close();
});
test('actual finish acknowledgment survives64-packet burst without queue trimming',async()=>{
 let sends=0;const{link,emit}=make({send:()=>sends++});for(let i=0;i<63;i++)emit(packet(4));emit(request(0xf0000000,0));
 const result=await new Updater({inputs:new Map(),outputs:new Map()}).serve(link,new Uint8Array(16),0xf0000000,100);
 assert.equal(result.finished,true);assert.equal(sends,1);link.close();
});
