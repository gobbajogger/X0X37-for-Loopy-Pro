// SPDX-License-Identifier: GPL-3.0-only
// Web MIDI connection-button feedback regression: native Node simulated devices,
// no firmware, real MIDI interface or flash actions.
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {createContext, Script} from 'node:vm';

const html = readFileSync(process.argv[2] || new URL('./x0x_installer.html', import.meta.url), 'utf8');
const segment = (a,b) => {
  const x=html.indexOf(a), y=html.indexOf(b,x);
  assert(x>=0 && y>x,`missing source ${a}`);
  return html.slice(x,y);
};
assert.match(html, /<div class="ctext" role="status" aria-live="polite" aria-atomic="true">/,
  'connection state and diagnostic should be announced, without stealing focus');
assert.match(html, /prefers-reduced-motion:\s*reduce/, 'respect reduced-motion preference');
for (const fragment of ['retryconnection: "Try again"','scanningconnection: "Scanning…"',
  'rescanconnection: "Rescan"','retryconnection: "もう一度接続"',
  'connectionnotfound:', 'connectiondenied:']) assert.ok(html.includes(fragment),fragment);
const buttons = {}, mock = {};
for (const id of ['connbtn','led','conn','connDetail']) {
  buttons[id] = {textContent:'',className:'',disabled:false,attrs:{},setAttribute(name,value){this.attrs[name]=value;}};
}
const stateFns = segment('let connectionState = "idle";', 'async function midiAccess()');
const scans = segment('let scanSerial = 0;', '$("connbtn").addEventListener("click"');
const ctx = createContext({buttons,mock});
new Script(`
const $=id=>buttons[id];
let connected=false, writing=false, lastId=null, lang='en';
const TEXT={ en: {connect:'Connect USB',retryconnection:'Try again',
  scanningconnection:'Scanning…',rescanconnection:'Rescan',connected:'Connected',
  notconnected:'Not connected',looking:'Looking…',error:'Error',
  connectionnotfound:'No keyboard. Try again',connectiondenied:'Allow MIDI then Try again'},
  ja: {connect:'USB 接続',retryconnection:'もう一度接続',scanningconnection:'検索中…',
  rescanconnection:'再検索', connected:'接続済み',notconnected:'未接続',looking:'探しています…'} };
const t = k => TEXT[lang][k];
let onFind = async () => null;
let findCalls=0;
async function midiAccess() {return {};}
class Updater {async find(predicate){findCalls++;return onFind(predicate);}}
function suggestModel() {}
${stateFns}
${scans}
globalThis.control={
 setLanguage(v){lang=v;renderConnectionAction();},
 setWriting(v){writing=v;renderConnectionAction();},
 connected(on,detail,bad){setConnected(on,detail,bad);},
 setFind(v){onFind=v;},
 async scan(){return scan();},
 getCalls(){return findCalls;}
};
`).runInContext(ctx);
const control=ctx.control;
control.connected(false,'USB unavailable',true);
assert.equal(buttons.connbtn.textContent,'Try again');
assert.equal(buttons.led.className,'led err');
assert.equal(buttons.connDetail.textContent,'USB unavailable');
assert.equal(buttons.connbtn.attrs['aria-disabled'],'false');
control.setLanguage('ja');
assert.equal(buttons.connbtn.textContent,'もう一度接続');
control.setLanguage('en');
control.connected(true,'SMK-37 Pro');
assert.equal(buttons.connbtn.textContent,'Rescan');
assert.equal(buttons.led.className,'led on');
console.log('PASS failure/connected state exposes retry and rescan, including Japanese');
control.setWriting(true);
assert.equal(buttons.connbtn.attrs['aria-disabled'],'true');
let calls=control.getCalls();
await control.scan();
assert.equal(control.getCalls(),calls,'scan refused while writing');
control.setWriting(false);
let resolveFind;
control.setFind(async () => await new Promise(res => { resolveFind=res; }));
const promise=control.scan();
assert.equal(buttons.connbtn.textContent,'Scanning…');
assert.equal(buttons.connbtn.attrs['aria-disabled'],'true');
assert.equal(buttons.connbtn.disabled,false,'button stays keyboard-focusable');
for (let n = 0; n < 12 && typeof resolveFind !== 'function'; n++) await Promise.resolve();
assert.equal(typeof resolveFind, 'function', 'pending mock find reached');
resolveFind({id:{model:'SMK-37 Pro',text:'SMK-37 Pro'},name:'test',link:{close(){}}});
await promise;
assert.equal(buttons.connbtn.textContent,'Rescan');
assert.equal(buttons.connbtn.attrs['aria-disabled'],'false');
console.log('PASS pending scan feedback, retained focusability, and guarded flash lock');
control.setFind(async () => null);
await control.scan();
assert.equal(buttons.connbtn.textContent,'Try again');
assert.equal(buttons.led.className,'led err');
assert.equal(buttons.connDetail.textContent,'No keyboard. Try again');
console.log('PASS missing keyboard shows actionable retry in place');
