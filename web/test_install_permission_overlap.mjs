// SPDX-License-Identifier: GPL-3.0-only
// Execute real scan/run/MIDI permission/button code; no hardware or firmware writes.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createContext, Script} from 'node:vm';
const html = readFileSync(process.argv[2] || new URL('./x0x_installer.html', import.meta.url), 'utf8');
function segment(a,b) {
  const x=html.indexOf(a), y=html.indexOf(b,x);
  assert(x>=0 && y>x, `missing production source ${a}`);
  return html.slice(x,y);
}
const runStart=html.indexOf('async function run(img, product, opts = {}) {');
const run=html.slice(runStart,html.indexOf('\n}',runStart)+2);
assert(runStart>=0 && run.endsWith('\n}'));
const connection=segment('let connectionState = "idle";', 'async function midiAccess()');
const midi=segment('async function midiAccess()', 'let scanSerial = 0;');
const scanning=segment('let scanSerial = 0;', '// Production approvals');
for (const previouslyConnected of [false,true]) {
  const requests=[], nodes={};
  const $=id=>nodes[id] ||= {textContent:'',checked:false,disabled:false,attrs:{},handlers:{},
    setAttribute(k,v){this.attrs[k]=v;}, addEventListener(k,f){this.handlers[k]=f;}};
  const navigator={requestMIDIAccess() {return new Promise((resolve,reject)=>requests.push({resolve,reject}));}};
  const labels={connect:'Connect USB',rescanconnection:'Rescan',scanningconnection:'Scanning…',
    retryconnection:'Try again',unsupportedaction:'Unavailable'};
  const ctx=createContext({$,navigator,setTimeout,clearTimeout,
    t:k=>labels[k] || k, sayK(){},log(){},updateInstallGate(){},refreshJourney(){},suggestModel(){},
    Updater:class {async find(){return null;}},stock:null});
  new Script(`let midi=null, connected=false, writing=false, lastId=null;\n${connection}\n${midi}\n${scanning}\n${run}\n`+
    `globalThis.control={run,connected:setConnected,isWriting:()=>writing,state:()=>connectionState};`).runInContext(ctx);
  if (previouslyConnected) ctx.control.connected(true,'SMK-37 Pro');
  $('connbtn').handlers.click(); // First real permission await belongs to scan().
  assert.equal(requests.length,1);
  assert.equal($('connbtn').textContent,'Scanning…');
  const install=ctx.control.run(new Uint8Array(),'SMK-37 Pro_900',{foreign:true});
  assert.equal(requests.length,2,'install has its own pending MIDI permission');
  assert.equal(ctx.control.isWriting(),true);
  requests[0].reject(new Error('scan permission denied'));
  for(let i=0;i<12;i++) await Promise.resolve();
  assert.equal(ctx.control.state(),'scanning','scan rejection during owned write must not change write UI');
  requests[1].reject(new Error('install permission denied'));
  await install;
  assert.equal(ctx.control.isWriting(),false,'permission rejection must release writer');
  assert.equal($('connbtn').textContent,previouslyConnected?'Rescan':'Connect USB',
    'early permission cleanup must recover the interrupted scan action');
  assert.equal($('connbtn').attrs['aria-disabled'],'false','Connect must allow retry after denial');
  assert.equal($('connbtn').disabled,false);
  $('connbtn').handlers.click(); // Actual registered guard must now admit a retry.
  assert.equal(requests.length,3,'real retry must request MIDI access again');
  requests[2].resolve({inputs:new Map(),outputs:new Map()});
  for(let i=0;i<16;i++) await Promise.resolve();
  assert.equal($('connbtn').textContent,'Try again');
  assert.equal($('connbtn').attrs['aria-disabled'],'false');
  console.log('PASS concurrent scan/install MIDI denials recover real Connect retry; prior connected='+previouslyConnected);
}
