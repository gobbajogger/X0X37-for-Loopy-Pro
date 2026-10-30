// SPDX-License-Identifier: GPL-3.0-only
// Exercise the actual WIP workspace source, with a browser DOM stub and no hardware.
import test from 'node:test';
import assert from 'node:assert/strict';
import { initDnboxWorkspace } from './workspace.js';

function fixture(pathname, hash) {
  const events = new Map();
  const sent = [];
  class Node {
    constructor(name='') {
      this.name = name; this.children=[]; this.attrs = new Map(); this.dataset={}; this.hidden=false;
      this.style={}; this.textContent=''; this.files=[]; this.value='';
    }
    getAttribute(k) { return this.attrs.get(k) ?? null; }
    setAttribute(k,v) { this.attrs.set(k,v); }
    removeAttribute(k) { this.attrs.delete(k); }
    replaceChildren() { this.children = []; }
    append(...nodes) { this.children.push(...nodes); }
    pause() { this.paused=true; }
  }
  const elements = new Map();
  const $ = (id) => {
    if (!elements.has(id)) elements.set(id,new Node(id));
    return elements.get(id);
  };
  for (const section of ['install','samples','projects']) {
    $(section+'view');
    const frame=$(section+'frame');
    frame.dataset.src=(pathname.includes('/install/') ? '../' : '')+(section === 'samples' ? 'breaks' : section)+'/';
    frame.contentWindow={postMessage(data, origin) { sent.push({section,data,origin}); }};
  }
  $('prepaudio');$('preprate').value='44100';$('prepspeed').value='2';
  $('preplist');$('prepstatus');
  const anchors=new Map(['install','samples','projects'].map(k=>[k,new Node('anchor_'+k)]));
  const document = {
    querySelector: sel => anchors.get(sel.match(/data-view="(.*?)"/)?.[1]),
    querySelectorAll: () => [],
    createElement: name => new Node(name),
  };
  const window={addEventListener:(name,callback)=>events.set(name,callback)};
  const location={pathname,hash,origin:'https://example.test'};
  const old={window:globalThis.window, document:globalThis.document, location:globalThis.location};
  Object.assign(globalThis,{window,document,location});
  initDnboxWorkspace({$,t:key=>key});
  return { $,anchors,events,sent,location,restore() {
    Object.assign(globalThis,old);
  }};
}

test('generated index.html selects generated sibling workspace, not developer source HTML',()=>{
  const f=fixture('/X0X37-for-Loopy-Pro/index.html','#samples');
  try {
    assert.equal(f.$('samplesview').hidden,false);
    assert.equal(f.$('samplesframe').src,'breaks/', 'generated root /index.html should select the real breaks workspace');
  }finally {f.restore()}
});
test('direct dev source x0x_installer.html still selects local source HTML',()=>{
  const f=fixture('/src/web/x0x_installer.html','#samples');
  try {assert.equal(f.$('samplesframe').src,'./x0x_breaks.html')}
  finally {f.restore()}
});
test('generated install/index.html deep link selects its ../projects/ frame',()=>{
  const f=fixture('/X0X37-for-Loopy-Pro/install/index.html','#projects');
  try {assert.equal(f.$('projectsframe').src,'../projects/', 'dataset src retains generated path')}
  finally {f.restore()}
});
test('wrong-origin messages cannot modify workspace iframe height',()=>{
  const f=fixture('/X0X37-for-Loopy-Pro/index.html','#samples');
  try {
    f.events.get('message')({origin:'https://attacker.test',source:f.$('samplesframe').contentWindow,data:{type:'dnbox-height',height:10000}});
    assert.equal(f.$('samplesframe').style.height,undefined);
    f.events.get('message')({origin:f.location.origin,source:f.$('samplesframe').contentWindow,data:{type:'dnbox-height',height:100000}});
    assert.equal(f.$('samplesframe').style.height,'20000px');
  }finally {f.restore()}
});
