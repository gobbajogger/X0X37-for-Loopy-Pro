// Real mountControls handlers against an event-driven DOM shim; no WASM/audio.
import test from 'node:test';
import assert from 'node:assert/strict';
import { mountControls } from './controls.js';

class Element {
  constructor() { this.children=[]; this.listeners=new Map(); this.attrs=new Map();
    this.style={}; this.value=''; this.textContent=''; this.disabled=false; }
  addEventListener(type, fn) { if(!this.listeners.has(type))this.listeners.set(type,[]);this.listeners.get(type).push(fn); }
  append(...children) { this.children.push(...children); }
  setAttribute(name, value) { this.attrs.set(name, String(value)); }
  getAttribute(name) { return this.attrs.get(name); }
  setPointerCapture() {}
  fire(type, extra={}) {
    const event={type, pointerId:1, key:' ', repeat:false, preventDefault(){}, ...extra};
    for(const fn of this.listeners.get(type) || []) fn(event);
  }
}
function fixture() {
  const ids=['dnbox-controls','dnbox-keys','dnbox-pads','dnbox-mixer',
    'dnbox-knobs','dnbox-play','dnbox-chord'];
  const els=new Map(ids.map(id=>[id,new Element()]));
  const events=[];
  const doc=new Element();
  doc.getElementById=id=>els.get(id);
  doc.createElement=()=>new Element();
  doc.dispatchEvent=event=>{
    if(event.type==='dnbox:input')events.push(event.detail);
    doc.fire(event.type,{detail:event.detail});
  };
  doc.hidden=false;
  const win=new Element();
  const controls=mountControls(doc,win);
  doc.fire('dnbox:booted',{detail:{board:1}});
  return {els,events,doc,win,controls,
    key:id=>els.get('dnbox-keys').children[id],
    pad:id=>els.get('dnbox-pads').children[id]};
}
const pair=(kind,id)=>[{kind,id,down:true},{kind,id,down:false}];

test('two independent fingers on the SAME accessible key, either release order',()=>{
  for (const order of [[101,202],[202,101]]) {
    const x=fixture(), key=x.key(12);
    key.fire('pointerdown',{pointerId:101});
    key.fire('pointerdown',{pointerId:202});
    assert.deepEqual(x.events,[pair('key',12)[0]]);
    assert.equal(key.getAttribute('aria-pressed'),'true');
    key.fire('pointerup',{pointerId:order[0]});
    assert.deepEqual(x.events,[pair('key',12)[0]]);
    assert.equal(key.getAttribute('aria-pressed'),'true');
    key.fire('lostpointercapture',{pointerId:order[0]});
    assert.deepEqual(x.events,[pair('key',12)[0]]);
    key.fire('pointerup',{pointerId:order[1]});
    assert.deepEqual(x.events,pair('key',12));
    assert.equal(key.getAttribute('aria-pressed'),'false');
  }
});
test('cancel first pointer, second pointer still held; cancel second releases once',()=>{
  const x=fixture(), key=x.key(0);
  key.fire('pointerdown',{pointerId:1});
  key.fire('pointerdown',{pointerId:2});
  key.fire('pointercancel',{pointerId:1});
  assert.deepEqual(x.events,[pair('key',0)[0]]);
  key.fire('pointercancel',{pointerId:2});
  key.fire('lostpointercapture',{pointerId:2});
  assert.deepEqual(x.events,pair('key',0));
  assert.equal(key.getAttribute('aria-pressed'),'false');
});
test('pointer and keyboard focus coexist on a key; keyboard blur leaves pointer held',()=>{
  const x=fixture(), key=x.key(12);
  key.fire('pointerdown',{pointerId:31});
  key.fire('keydown',{key:' ',repeat:false});
  key.fire('blur');
  assert.deepEqual(x.events,[pair('key',12)[0]]);
  assert.equal(key.getAttribute('aria-pressed'),'true');
  key.fire('pointerup',{pointerId:31});
  assert.deepEqual(x.events,pair('key',12));
});
test('keyboard Space and Enter own distinct holds, repeat and keyup stay paired',()=>{
  const x=fixture(), key=x.key(19);
  key.fire('keydown',{key:' '});
  key.fire('keydown',{key:' ',repeat:true});
  key.fire('keydown',{key:'Enter'});
  key.fire('keyup',{key:' '});
  assert.deepEqual(x.events,[pair('key',19)[0]]);
  key.fire('keyup',{key:'Enter'});
  assert.deepEqual(x.events,pair('key',19));
  assert.equal(key.getAttribute('aria-pressed'),'false');
});
test('window blur, hide, reboot disable and re-enable clean every pointer and aria state',()=>{
  const x=fixture(), key=x.key(12), pad=x.pad(2);
  key.fire('pointerdown',{pointerId:2});
  pad.fire('pointerdown',{pointerId:3});
  x.win.fire('blur');
  assert.deepEqual(x.events,[{kind:'key',id:12,down:true},{kind:'pad',id:2,down:true},
    {kind:'key',id:12,down:false},{kind:'pad',id:2,down:false}]);
  assert.equal(key.getAttribute('aria-pressed'),'false');
  assert.equal(pad.getAttribute('aria-pressed'),'false');
  key.fire('pointerup',{pointerId:2});
  pad.fire('pointerup',{pointerId:3});
  assert.equal(x.events.length,4);
  key.fire('pointerdown',{pointerId:44});
  x.doc.hidden=true; x.doc.fire('visibilitychange');
  assert.equal(key.getAttribute('aria-pressed'),'false');
  x.doc.fire('dnbox:booting');
  assert.equal(key.disabled,true);
  x.doc.fire('dnbox:booted',{detail:{board:1}});
  assert.equal(key.disabled,false);
  key.fire('pointerdown',{pointerId:44});
  key.fire('pointerup',{pointerId:44});
  assert.deepEqual(x.events.slice(-3),[
    {kind:'key',id:12,down:false},{kind:'key',id:12,down:true},{kind:'key',id:12,down:false}]);
});
test('the 16th pad and SEQ PLAY are counted momentaries, not duplicate events',()=>{
  const x=fixture();
  const pad=x.pad(15);
  pad.fire('pointerdown',{pointerId:11});
  pad.fire('pointerdown',{pointerId:12});
  pad.fire('pointerup',{pointerId:11});
  assert.deepEqual(x.events,[pair('pad',15)[0]]);
  pad.fire('pointerup',{pointerId:12});
  assert.deepEqual(x.events,pair('pad',15));
  const play=x.els.get('dnbox-play');
  play.fire('pointerdown',{pointerId:23});
  play.fire('pointerup',{pointerId:23});
  assert.deepEqual(x.events.slice(-2),pair('button',53));
});
