// SPDX-License-Identifier: GPL-3.0-only
// Browser installer post-write status regression. Mocked MIDI, no hardware writes.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {Script,createContext} from 'node:vm';
const source=readFileSync(process.argv[2] || new URL('./x0x_installer.html',import.meta.url),'utf8');
function extract(anchor){
 const start=source.indexOf(anchor),end=source.indexOf('\n}',start);
 assert.ok(start>=0 && end>start,'installer function missing: '+anchor);
 return source.slice(start,end+2);
}
const scan=extract('async function scan() {');
const run=extract('async function run(img, product, opts = {}) {');
assert.match(scan,/if \(writing\) return;/,'active-write scan guard must remain');
for(const mode of ['normal','resume','failure']){
 const state={connected:false,scans:0,leds:[],status:'',written:false};
 const nodes=Object.fromEntries(['go','stockgo','stockconfirm','conn','bar'].map(k=>[k,{disabled:false,checked:false,value:0,textContent:''}]));
 const link={close(){}};
 const sandbox={
  Blob, URL, setTimeout, document:{createElement(){return {click(){},remove(){}}},body:{appendChild(){}}},
  $:k=>nodes[k]||(nodes[k]={}),midiAccess:async()=>({}),
  Updater:class {
   async find(pred){
    const ota={model:'ota-SMK-37 Pro',text:'ota-SMK-37 Pro'};
    const app={model:'SMK-37 Pro',text:'SMK-37 Pro'};
    if(mode==='resume'&&!state.written&&pred(ota))return {id:ota,link};
    if(state.written&&pred(app))return {id:app,link};
    return null;
   }
   async resume(){state.written=true;return true}
  },
  installKeepingSlots:async()=>{
   if(mode==='failure')throw Object.assign(new Error('test write failed'),{code:'lost'});
   state.written=true;return {move:null};
  },
  t:k=>k,sayK:k=>{state.status=k},log(){},led:k=>state.leds.push(k),
  setConnected:on=>{state.connected=on;state.scans++;state.leds.push(on?'on':'off')},suggestModel(){},sameKeyboard(){return true},
  updateInstallGate(){state.gateUpdated=true},stock:null,TEXT:{en:{lost:'USB lost'}},lang:'en'
 };
 const context=createContext(sandbox);
 new Script('let writing=false,lastId=null;\n'+scan+'\n'+run+'\nglobalThis.start=run;globalThis.isWriting=()=>writing;').runInContext(context);
 await context.start(new Uint8Array(0),'SMK-37 Pro_900');
 assert.equal(context.isWriting(),false,mode+': write lock');
 assert.equal(state.gateUpdated,true,mode+': gate refresh');
 if(mode==='failure'){
  assert.equal(state.scans,0,'failed write must not mask error with a reconnect');
  assert.equal(state.leds.at(-1),'err');
 }else{
  assert.equal(state.scans,1,mode+': missing post-write scan');
  assert.equal(state.connected,true,mode+': stale connection status');
  assert.equal(state.leds.at(-1),'on',mode+': stale busy LED');
 }
 console.log('PASS '+mode+': reconnects='+state.scans+'; last LED='+state.leds.at(-1));
}
console.log('PASS three mocked post-write installer connection scenarios');
