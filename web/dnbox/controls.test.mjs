import test from 'node:test';
import assert from 'node:assert/strict';
import {KEY_COUNT, PAD_COUNT, MIDI_BASE, noteAt, heldRouter} from './controls.js';

test('every SMK-37 note maps exactly C3 (48) through C6 (84)',()=>{
  assert.equal(KEY_COUNT,37); assert.equal(PAD_COUNT,16); assert.equal(MIDI_BASE,48);
  const notes=Array.from({length:KEY_COUNT},(_,i)=>noteAt(i));
  assert.equal(notes[0].label,'C3'); assert.equal(notes[12].label,'C4');
  assert.equal(notes[24].label,'C5'); assert.equal(notes[36].label,'C6');
  assert.equal(notes.filter(n=>n.black).length,15);
  assert.equal(notes.filter(n=>!n.black).length,22);
  assert.equal(new Set(notes.map(n=>n.midi)).size,37);
  assert.throws(()=>noteAt(-1)); assert.throws(()=>noteAt(37)); assert.throws(()=>noteAt(0.1));
});
test('chords keep separate holds, exactly paired release and cancel',()=>{
  const events=[];const h=heldRouter(e=>events.push(e));
  assert.equal(h.press('one','key',12),true); // C4
  assert.equal(h.press('two','key',15),true); // Eb4
  assert.equal(h.press('three','key',19),true);// G4
  assert.equal(h.press('one','key',12),false); // repeated pointer/keypress ignored
  assert.equal(h.count,3);
  assert.equal(h.release('two'),true);
  h.releaseAll();
  assert.equal(h.count,0);
  assert.deepEqual(events.map(e=>[e.kind,e.id,e.down]),[
    ['key',12,true],['key',15,true],['key',19,true],
    ['key',15,false],['key',12,false],['key',19,false]
  ]);
  assert.equal(h.release('three'),false);
});
test('pad and transport stay hardware events, not synthetic audio',()=>{
  const events=[];const h=heldRouter(e=>events.push(e));
  h.press('p','pad',15);h.release('p');
  h.press('b','button',53);h.release('b');
  assert.deepEqual(events,[
    {kind:'pad',id:15,down:true},{kind:'pad',id:15,down:false},
    {kind:'button',id:53,down:true},{kind:'button',id:53,down:false}
  ]);
  for(const [kind,id] of [['pad',16],['key',37],['button',36],['button',55]]) {
    assert.throws(()=>h.press('bad',kind,id));
  }
});
