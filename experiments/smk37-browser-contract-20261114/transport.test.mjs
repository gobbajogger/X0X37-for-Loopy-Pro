import test from 'node:test';
import assert from 'node:assert/strict';
import {FakeMidiEndpoint, MidiContract} from './transport.mjs';
const rig = options => { const endpoint=new FakeMidiEndpoint(options); const contract=new MidiContract(); contract.connect(endpoint); return {endpoint,contract}; };

test('initial state and fake-only admission',()=>{
  const c=new MidiContract(); assert.equal(c.status.state,'disconnected'); assert.throws(()=>c.arm());
  assert.throws(()=>c.connect({connected:true,send(){throw Error('must not execute');}}));
  const e=new FakeMidiEndpoint(); c.connect(e); assert.equal(c.status.state,'connected-disarmed'); assert.deepEqual(e.sent,[]);
  assert.throws(()=>c.noteOn(60)); assert.deepEqual(e.sent,[]);
});
test('exact musical bytes and duplicate local press',()=>{
  const {endpoint:e,contract:c}=rig(); c.arm(); c.noteOn(60,101); assert.equal(c.noteOn(60),false);
  c.controlChange(1,75); c.controlChange(64,127); c.pitchBend(16383); c.noteOff(60); c.controlChange(64,0); c.pitchBend(8192);
  assert.deepEqual(e.sent,[[0x91,60,101],[0xB1,1,75],[0xB1,64,127],[0xE1,127,127],[0x81,60,0],[0xB1,64,0],[0xE1,0,64]]);
});
test('channel change releases notes and performance controls on old channel',()=>{
  const {endpoint:e,contract:c}=rig(); c.arm(); c.noteOn(60); c.controlChange(64,90); c.pitchBend(0); c.setChannel(3);
  assert.deepEqual(e.sent.slice(-3),[[0x81,60,0],[0xB1,64,0],[0xE1,0,64]]);
  assert.equal(c.status.channel,3); assert.equal(c.status.armed,false); assert.throws(()=>c.noteOn(61));
  c.arm();c.noteOn(61);assert.deepEqual(e.sent.at(-1),[0x92,61,100]);
});
test('explicit cleanup owns only its tracked notes and transport',()=>{
  const {endpoint:e,contract:c}=rig(); c.arm(); c.noteOn(0); c.noteOn(127); c.realtime('start'); c.realtime('clock'); c.disarm();
  assert.deepEqual(e.sent.slice(-3),[[0x81,0,0],[0x81,127,0],[0xFC]]); const n=e.sent.length;c.disarm();assert.equal(e.sent.length,n);
});
test('all four realtime bytes; F2 and non allowlisted operations reject',()=>{
  const {endpoint:e,contract:c}=rig(); c.arm(); for(const x of ['clock','start','continue','stop'])c.realtime(x);
  assert.deepEqual(e.sent,[[0xF8],[0xFA],[0xFB],[0xFC]]);
  for(const x of ['position','sysex','program','toString','constructor','__proto__']) assert.throws(()=>c.realtime(x));
  assert.equal(e.sent.length,4);
});
test('unsupported controls and invalid values do not mutate or transmit',()=>{
  const {endpoint:e,contract:c}=rig();c.arm();c.noteOn(60);
  for(const cc of [0,7,10,11,32,74,98,99,120,123])assert.throws(()=>c.controlChange(cc,0));
  for(const x of [0,16,17,1.5,'3',NaN,Infinity])assert.throws(()=>c.setChannel(x));
  for(const f of [()=>c.noteOn(-1),()=>c.noteOn(60,0),()=>c.noteOn(128),()=>c.pitchBend(16384),()=>c.controlChange(1,128),()=>c.unsupported('PROJECT'),()=>c.unsupported('WATCH')])assert.throws(f);
  assert.equal(c.status.channel,2);assert.deepEqual(c.status.held,[60]);assert.equal(e.sent.length,1);
});
test('input feedback has no echo, no ownership effect and clears on lifecycle',()=>{
  const {endpoint:e,contract:c}=rig({loopback:true});c.arm();c.noteOn(60);e.receive([0x81,60,0]);
  assert.deepEqual(c.status.held,[60]);assert.equal(e.sent.length,1);assert.equal(c.status.feedback.length,2);
  e.receive([0x92,61,100]);e.receive([0xB1,74,10]);e.receive([0xF0,0x7D,0xF7]);e.receive([0x91,128,2]);
  assert.equal(c.status.feedback.length,2);c.disarm();assert.equal(c.status.feedback.length,0);assert.equal(e.sent.length,2);
  e.receive([0x91,60,100]);assert.equal(c.status.feedback.length,0);
});
test('observation and logs are bounded and returned arrays are defensive copies',()=>{
  const {endpoint:e,contract:c}=rig();c.arm();for(let i=0;i<200;i++)e.receive([0x91,60,100]);
  assert.equal(c.status.feedback.length,128);assert.equal(c.log.length,128);
  const s=c.status;s.feedback[0].bytes[1]=0;assert.equal(c.status.feedback[0].bytes[1],60);
});
test('planned disconnect releases before detaching and reconnect requires arm',()=>{
  const {endpoint:e,contract:c}=rig();c.arm();c.noteOn(60);c.disconnect();assert.deepEqual(e.sent.at(-1),[0x81,60,0]);
  assert.equal(c.status.state,'disconnected');const replacement=new FakeMidiEndpoint();c.connect(replacement);assert.throws(()=>c.noteOn(60));assert.deepEqual(replacement.sent,[]);
});
test('abrupt loss exposes uncertainty and old session never replays',()=>{
  const {endpoint:e,contract:c}=rig();c.arm();c.noteOn(60);e.lose();assert.equal(c.status.state,'disconnected');assert.equal(c.status.releaseUnconfirmed,true);
  const replacement=new FakeMidiEndpoint();c.connect(replacement);c.arm();c.disarm();assert.deepEqual(replacement.sent,[]);assert.equal(c.status.releaseUnconfirmed,true);
});
test('send-then-throw triggers safety release and faults until reconnect',()=>{
  const {endpoint:e,contract:c}=rig();c.arm();e.failNext('after');assert.throws(()=>c.noteOn(60));
  assert.deepEqual(e.sent,[[0x91,60,100],[0x81,60,0]]);assert.equal(c.status.state,'fault');assert.equal(c.status.releaseUnconfirmed,true);assert.throws(()=>c.arm());
});
test('cleanup continues after an independent release failure',()=>{
  const {endpoint:e,contract:c}=rig();c.arm();c.noteOn(60);c.noteOn(61);c.controlChange(64,127);c.pitchBend(0);c.realtime('continue');
  e.failNext();c.disarm();assert.deepEqual(e.sent.slice(-4),[[0x81,61,0],[0xB1,64,0],[0xE1,0,64],[0xFC]]);assert.equal(c.status.releaseUnconfirmed,true);
});
test('endpoint replacement cleans original then ignores old input',()=>{
  const {endpoint:e,contract:c}=rig();c.arm();c.noteOn(60);const second=new FakeMidiEndpoint();c.connect(second);
  assert.deepEqual(e.sent.at(-1),[0x81,60,0]);assert.deepEqual(second.sent,[]);c.arm();e.receive([0x91,60,100]);e.lose();assert.equal(c.status.state,'armed');assert.equal(c.status.feedback.length,0);
});
test('10 second absolute lease covers held controls and cannot be prolonged by traffic or arm',()=>{
  let time=0;const c=new MidiContract({now:()=>time});const e=new FakeMidiEndpoint();c.connect(e);c.arm();c.controlChange(64,127);c.pitchBend(0);c.realtime('start');
  time=9000;c.realtime('clock');c.arm();time=10000;c.poll();assert.equal(c.status.armed,false);assert.deepEqual(e.sent.slice(-3),[[0xB1,64,0],[0xE1,0,64],[0xFC]]);
});
test('clock rollback releases instead of extending lease',()=>{
  let time=50;const c=new MidiContract({now:()=>time});const e=new FakeMidiEndpoint();c.connect(e);c.arm();c.noteOn(60);time=49;c.poll();assert.equal(c.status.armed,false);assert.deepEqual(e.sent.at(-1),[0x81,60,0]);
});
test('lifecycle guards survive replacement and detached guards cannot release',()=>{
  const {endpoint:e,contract:c}=rig();const target=new EventTarget();const doc=new EventTarget();doc.hidden=true;
  const detach=c.bindLifecycle(target,doc);c.arm();c.noteOn(60);target.dispatchEvent(new Event('blur'));assert.equal(c.status.armed,false);
  const next=new FakeMidiEndpoint();c.connect(next);c.arm();c.noteOn(61);target.dispatchEvent(new Event('blur'));assert.equal(c.status.armed,false);detach();
  c.arm();c.noteOn(62);target.dispatchEvent(new Event('blur'));assert.equal(c.status.armed,true);
  const detach2=c.bindLifecycle(target,doc);doc.dispatchEvent(new Event('visibilitychange'));assert.equal(c.status.armed,false);detach2();
});
