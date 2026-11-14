// Independent synthetic transport acceptance probes. No MIDI API or devices.
import test from 'node:test';
import assert from 'node:assert/strict';
import {FakeMidiEndpoint, MidiContract} from '../transport.mjs';

function rig(options={}) {
  let time=0;
  const midi=new MidiContract({now:()=>time});
  const endpoint=new FakeMidiEndpoint(options);
  midi.connect(endpoint);
  return {midi,endpoint,setTime:t=>{time=t;}};
}

test('replacement releases notes, pedal, bend and locally started transport on old route only',()=>{
  const {midi,endpoint}=rig(); midi.arm();
  midi.noteOn(60,100); midi.noteOn(64,100); midi.controlChange(64,127); midi.pitchBend(16383); midi.realtime('start');
  const replacement=new FakeMidiEndpoint(); midi.connect(replacement);
  assert.deepEqual(endpoint.sent.slice(-5),[[0x81,60,0],[0x81,64,0],[0xB1,64,0],[0xE1,0,64],[0xFC]]);
  assert.deepEqual(replacement.sent,[]); assert.equal(midi.status.armed,false);
  midi.arm(); assert.deepEqual(replacement.sent,[]);
});

test('channel transition and duplicate downs preserve a balanced original-channel release',()=>{
  const {midi,endpoint}=rig(); midi.arm();
  assert.equal(midi.noteOn(60,127),true); assert.equal(midi.noteOn(60,120),false);
  midi.setChannel(3); assert.deepEqual(endpoint.sent,[[0x91,60,127],[0x81,60,0]]);
  assert.equal(midi.status.channel,3); assert.equal(midi.status.armed,false);
  assert.throws(()=>midi.noteOn(60),/arm/); midi.arm(); midi.noteOn(60); midi.noteOff(60);
  assert.deepEqual(endpoint.sent.slice(-2),[[0x92,60,100],[0x82,60,0]]);
});

test('invalid values and unsupported semantic operations leave bytes and state unchanged',()=>{
  const {midi,endpoint}=rig(); midi.arm(); midi.noteOn(64);
  const before=midi.status, bytes=endpoint.sent;
  for(const action of [()=>midi.setChannel(0),()=>midi.setChannel(16),()=>midi.setChannel(2.2),()=>midi.noteOn(-1),()=>midi.noteOn(128),()=>midi.noteOn(65,0),()=>midi.noteOn(65,128),()=>midi.noteOff(NaN),()=>midi.controlChange(123,0),()=>midi.controlChange(120,0),()=>midi.controlChange(7,127),()=>midi.pitchBend(16384),()=>midi.realtime('position'),()=>midi.unsupported('WATCH'),()=>midi.unsupported('SysEx'),()=>midi.unsupported('program')]) assert.throws(action);
  assert.deepEqual(endpoint.sent,bytes); assert.deepEqual(midi.status,before);
});

test('prototype property names are unsupported real-time operations',()=>{
  for(const operation of ['toString','constructor','__proto__','valueOf','hasOwnProperty']) {
    const {midi,endpoint}=rig(); midi.arm();
    assert.throws(()=>midi.realtime(operation),/unsupported/);
    assert.deepEqual(endpoint.sent,[]);
  }
});

test('fake-only admission rejects duck typing and prototype-copy objects',()=>{
  const midi=new MidiContract();
  for(const endpoint of [{connected:true,send(){throw Error('must not call');},subscribe(){}},Object.create(FakeMidiEndpoint.prototype)]) assert.throws(()=>midi.connect(endpoint));
  assert.equal(midi.status.state,'disconnected');
});

test('fake-only execution cannot dispatch a subclass override',()=>{
  let foreignCalls=0;
  class Foreign extends FakeMidiEndpoint { send(){foreignCalls++;} }
  const midi=new MidiContract();
  try { const endpoint=new Foreign(); midi.connect(endpoint); midi.arm(); midi.noteOn(60); } catch {}
  assert.equal(foreignCalls,0);
});

test('fake-only execution cannot dispatch a patched send method',()=>{
  let foreignCalls=0;
  const endpoint=new FakeMidiEndpoint();
  try { endpoint.send=()=>{foreignCalls++;}; } catch {}
  const midi=new MidiContract(); midi.connect(endpoint); midi.arm(); midi.noteOn(60);
  assert.equal(foreignCalls,0);
});

for(const failure of ['before','after']) test(`Note On ${failure}-enqueue exception attempts OFF and keeps uncertainty across reconnect`,()=>{
  const {midi,endpoint}=rig(); midi.arm(); endpoint.failNext(failure);
  assert.throws(()=>midi.noteOn(60),/injected failure/);
  assert.deepEqual(endpoint.sent.at(-1),[0x81,60,0]);
  assert.equal(midi.status.armed,false); assert.equal(midi.status.releaseUnconfirmed,true); assert.equal(midi.status.state,'fault');
  assert.throws(()=>midi.arm());
  const replacement=new FakeMidiEndpoint(); midi.connect(replacement); midi.arm();
  assert.equal(midi.status.releaseUnconfirmed,true); assert.deepEqual(replacement.sent,[]);
});

test('first cleanup failure does not skip remaining obligations',()=>{
  const {midi,endpoint}=rig(); midi.arm(); midi.noteOn(60); midi.noteOn(61); midi.controlChange(64,127); midi.pitchBend(0); midi.realtime('continue');
  endpoint.failNext('before'); midi.disarm('hostile test');
  assert.deepEqual(endpoint.sent.slice(-4),[[0x81,61,0],[0xB1,64,0],[0xE1,0,64],[0xFC]]);
  assert.deepEqual(midi.status.held,[]); assert.equal(midi.status.releaseUnconfirmed,true);
  const count=endpoint.sent.length; midi.disarm(); assert.equal(endpoint.sent.length,count);
});

test('lost endpoint yields truthful uncertainty and never replays cleanup on replacement',()=>{
  const {midi,endpoint}=rig(); midi.arm(); midi.noteOn(60); midi.controlChange(64,127); endpoint.lose();
  assert.equal(midi.status.state,'disconnected'); assert.equal(midi.status.releaseUnconfirmed,true);
  const replacement=new FakeMidiEndpoint(); midi.connect(replacement); midi.arm();
  assert.deepEqual(replacement.sent,[]); endpoint.receive([0x91,60,100]); endpoint.lose();
  assert.equal(midi.status.state,'armed'); assert.deepEqual(midi.status.feedback,[]);
});

test('looped input is observation-only and is never tracked as a local release obligation',()=>{
  const {midi,endpoint}=rig({loopback:true}); midi.arm(); midi.noteOn(60);
  assert.equal(endpoint.sent.length,1); assert.equal(midi.status.feedback.length,1);
  endpoint.receive([0x91,61,100]); endpoint.receive([0xB1,64,127]); endpoint.receive([0xFA]);
  assert.equal(endpoint.sent.length,1); midi.noteOff(60); midi.disarm();
  assert.deepEqual(endpoint.sent,[[0x91,60,100],[0x81,60,0]]);
  assert.deepEqual(midi.status.feedback,[]);
});

test('wrong-channel, unsupported and malformed inbound messages cannot mutate observations',()=>{
  const {midi,endpoint}=rig(); midi.arm();
  for(const bytes of [[0x92,60,100],[0xC1,1],[0xD1,1],[0xA1,60,100],[0xB1,7,100],[0xB1,123,0],[0xF0,0x7D,0xF7],[0xF2,0,0],[0x91,128,1],[0x91,60,-1],[0x91,60,NaN],[0xFA,0],[0x91,60],[]]) endpoint.receive(bytes);
  assert.deepEqual(midi.status.feedback,[]); assert.deepEqual(endpoint.sent,[]); assert.deepEqual(midi.status.held,[]);
});

test('observations are bounded and defensive copies cannot mutate internal state',()=>{
  const {midi,endpoint}=rig(); midi.arm();
  for(let i=0;i<300;i++) endpoint.receive([0x91,i%128,100]);
  assert.equal(midi.status.feedback.length,128); assert.ok(midi.log.length<=128);
  const exposed=midi.status; exposed.feedback[0].bytes[0]=0xF0; exposed.held.push(127);
  assert.equal(midi.status.feedback[0].bytes[0],0x91); assert.deepEqual(midi.status.held,[]);
  midi.disarm(); endpoint.receive([0x91,60,100]); assert.deepEqual(midi.status.feedback,[]);
});

for(const obligation of ['note','pedal','bend','transport']) test(`absolute arm lease bounds ${obligation} even during continuous clock and repeated arm`,()=>{
  const {midi,endpoint,setTime}=rig(); midi.arm();
  if(obligation==='note') midi.noteOn(60);
  if(obligation==='pedal') midi.controlChange(64,127);
  if(obligation==='bend') midi.pitchBend(0);
  if(obligation==='transport') midi.realtime('start');
  for(const time of [1000,5000,9000]) { setTime(time); midi.realtime('clock'); midi.arm(); }
  setTime(10000); midi.poll();
  assert.equal(midi.status.armed,false); assert.deepEqual(midi.status.held,[]);
  const release={note:[0x81,60,0],pedal:[0xB1,64,0],bend:[0xE1,0,64],transport:[0xFC]}[obligation];
  assert.deepEqual(endpoint.sent.at(-1),release);
});

test('clock rollback disarms and releases tracked obligations',()=>{
  const {midi,endpoint,setTime}=rig(); setTime(100); midi.arm(); midi.noteOn(60); setTime(0); midi.poll();
  assert.equal(midi.status.armed,false); assert.deepEqual(endpoint.sent.at(-1),[0x81,60,0]);
});

test('fresh sends enforce the arm lease even before the next periodic poll',()=>{
  const {midi,endpoint,setTime}=rig(); midi.arm(); midi.noteOn(60); setTime(10000);
  assert.throws(()=>midi.noteOn(61),/arm/);
  assert.equal(midi.status.armed,false); assert.deepEqual(endpoint.sent,[[0x91,60,100],[0x81,60,0]]);
});

test('lifecycle binding remains effective after replacement and detach removes it',()=>{
  const {midi,endpoint}=rig(); const target=new EventTarget(), doc=new EventTarget(); doc.hidden=false;
  const unbind=midi.bindLifecycle(target,doc); midi.arm(); midi.noteOn(60); target.dispatchEvent(new Event('blur'));
  assert.equal(midi.status.armed,false); assert.deepEqual(endpoint.sent.at(-1),[0x81,60,0]);
  const replacement=new FakeMidiEndpoint(); midi.connect(replacement); midi.arm(); midi.noteOn(61); doc.hidden=true; doc.dispatchEvent(new Event('visibilitychange'));
  assert.equal(midi.status.armed,false); assert.deepEqual(replacement.sent.at(-1),[0x81,61,0]);
  unbind(); midi.arm(); target.dispatchEvent(new Event('blur')); assert.equal(midi.status.armed,true);
});

test('Escape and pagehide disarm without echo or untracked releases',()=>{
  const {midi,endpoint}=rig(); const target=new EventTarget(); const unbind=midi.bindLifecycle(target);
  midi.arm(); midi.noteOn(60); const escape=new Event('keydown'); escape.key='Escape'; target.dispatchEvent(escape);
  assert.equal(midi.status.armed,false); midi.arm(); midi.noteOn(61); target.dispatchEvent(new Event('pagehide'));
  assert.equal(midi.status.armed,false); assert.deepEqual(endpoint.sent,[[0x91,60,100],[0x81,60,0],[0x91,61,100],[0x81,61,0]]); unbind();
});
