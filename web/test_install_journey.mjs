#!/usr/bin/env node
// Static contract + live projection of the production journey function (no Web MIDI writes).
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
const source=readFileSync(process.argv[2]||'web/x0x_installer.html','utf8');
const block=source.match(/function refreshJourney\(\) \{[\s\S]*?\n\}/);
assert.ok(block,'workflow status projection exists');
assert.match(source,/\.journey li\[data-state="current"\]/);
assert.match(source,/<li data-step="connect" data-state="current" aria-current="step">/);
assert.match(source,/aria-live="polite"/);
assert.match(source,/function setConnected\([\s\S]*?refreshJourney\(\);/);
assert.match(source,/function updateInstallGate\([\s\S]*?refreshJourney\(\);/);
assert.match(source,/stock = \{ image: logicalImage\(raw\)[\s\S]*?refreshJourney\(\);/);
const items=Array.from({length:3},()=>({dataset:{},attrs:{},setAttribute(k,v){this.attrs[k]=v},removeAttribute(k){delete this.attrs[k]},getAttribute(k){return this.attrs[k]}}));
const document={querySelectorAll(q){assert.equal(q,'.journey li[data-step]');return items;}};
const render=new Function('document','connected','writing','stock','image','selectedProduct',block[0]+';refreshJourney();');
function check(name,connected,writing,stock,image,selectedProduct,want){
  render(document,connected,writing,stock,image,selectedProduct);
  assert.deepEqual(items.map(i=>i.dataset.state),want,name);
  assert.equal(items.filter(i=>i.attrs['aria-current']==='step').length,1,name+' has exactly one current step');
  assert.equal(items.findIndex(i=>i.attrs['aria-current']==='step'),want.indexOf('current'),name+' current step position');
}
check('initial',false,false,null,null,null,['current','pending','pending']);
check('connected no package',true,false,null,null,null,['complete','current','pending']);
check('selected while offline',false,false,{product:'SMK-37 Pro'},null,null,['current','complete','pending']);
check('ready for review',true,false,{product:'SMK-37 Pro'},null,null,['complete','complete','current']);
check('catalog package ready',true,false,null,new Uint8Array(2),'SMK-37 Pro',['complete','complete','current']);
check('invalid replacement',true,false,null,null,null,['complete','current','pending']);
check('write with link lost',false,true,null,null,null,['pending','pending','current']);
console.log('PASS 7 journey projection cases + one aria-current after every state');
