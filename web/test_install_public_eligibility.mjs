// SPDX-License-Identifier: GPL-3.0-only
// Real source excerpts; no MIDI, firmware download, write, backup or device access.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { Script, createContext } from 'node:vm';
const path = process.argv[2] || new URL('./x0x_installer.html', import.meta.url);
const html = readFileSync(path, 'utf8');
function cut(begin,end){const a=html.indexOf(begin),b=html.indexOf(end,a);assert(a>=0&&b>a,`${begin} source missing`);return html.slice(a,b);}
const admission = cut('const PRODUCTION_DEVICE_APPROVALS', '\nfunction currentVersion()');
const checker = cut('function isCatalogChecked(', '\nfunction updateInstallGate()');
const history = cut('function renderBuildHistory(', '\nasync function sha256Hex(');
const chooser = cut('function fillVersions()', '\nfunction confirmWrite(');
const releases = cut('async function mergeGitHubReleases()', '\nlet ctlLoadSerial');
assert.match(releases, /never become production picker/);
assert.doesNotMatch(releases, /fetch\s*\(/, 'unreviewed GitHub Releases must not be contacted');
assert.match(history,/const ineligible/, 'missing read-only unapproved history');
assert.match(chooser, /renderBuildHistory\(\).*Show non-installable history/, 'empty picker hides archived history');
assert.match(chooser,/\$\("download"\)\.removeAttribute\("href"\)/, 'empty picker keeps unsafe download link');
assert.match(checker,/approvedModels\(v\)/, 'installed guard does not check publisher-approved models');

const doc = {getElementById: () => null};
const context=createContext({Object,Number,Array, console, document:doc});
new Script(`let catalogLive={versions:[]},chosenModel='elite';\n${admission}\n${checker}\nglobalThis.evaluate=(v,m,ap)=>approvedModels(v,ap);globalThis.getPicker=()=>versions();globalThis.testCheck=(v,m)=>isCatalogChecked(v,m);globalThis.setCatalog=v=>catalogLive=v;globalThis.setModel=v=>chosenModel=v;`).runInContext(context);
const E='elite',P='pro',VER='dev-20261022-1638-e894804';
const pE={product:'SMK-37 Elite_900',pkg:'../firmware/test-elite.fwsc',size:651396,sha256:'a'.repeat(64),controller_sha256:'b'.repeat(64)};
const pP={product:'SMK-37 Pro_900',pkg:'../firmware/test-pro.fwsc',size:651392,sha256:'c'.repeat(64),controller_sha256:'d'.repeat(64)};
const url='https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/123/hardware-pass.md';
const rE={hardware_pass:true,sha256:'e'.repeat(64),url};
const rP={hardware_pass:true,sha256:'f'.repeat(64),url};
const good={version:VER, status:'dev-checked',lifecycle:'current',hardware_tested:true,owner_publication_approved:true,source_commit:'1'.repeat(40),models:{elite:pE,pro:pP},device_evidence:{elite:rE,pro:rP}};
const approval={owner_publication_approved:true,model:E,product:pE.product,source_commit:good.source_commit,package_sha256:pE.sha256,controller_sha256:pE.controller_sha256,device_report_sha256:rE.sha256,device_report_url:url};
const allow={[`${VER}/elite`]:approval};
function evaluated(v=good,ap=allow){return Array.from(context.evaluate(v,'elite',ap));}
function rejects(name, mutate,ap=allow){const copy=structuredClone(good);mutate(copy);assert.deepEqual(evaluated(copy,ap),[],name);console.log('PASS refuse '+name)}
assert.deepEqual(evaluated(),[E], 'independent positive test for correct elite exact-byte tuple');
console.log('PASS verified Elite only (Pro remains hidden without separate proof)');
assert.deepEqual(evaluated(good,{}),[],'compiled empty allowlist must override catalog');
context.setCatalog({latest:VER,versions:[good]});
assert.equal(context.getPicker().length,0,'default production picker MUST be empty with no independently reviewed approvals');
assert.equal(context.testCheck(good,pE),false,'UI write gate must stay disabled');
console.log('PASS production runtime rejects even forged all-true catalog');
rejects('hardware false',v=>v.hardware_tested=false);
rejects('hardware bool forgery with empty approval map',v=>{},{});
rejects('missing owner publication approval',v=>v.owner_publication_approved=false);
rejects('missing evidence',v=>delete v.device_evidence.elite);
rejects('missing per-model physical PASS',v=>v.device_evidence.elite.hardware_pass=false);
rejects('wrong package model',v=>v.models.elite.product=pP.product);
rejects('wrong sha256',v=>v.models.elite.sha256='d'.repeat(64));
rejects('wrong controller sha',v=>v.models.elite.controller_sha256='0'.repeat(64));
rejects('wrong report hash',v=>v.device_evidence.elite.sha256='0'.repeat(64));
rejects('wrong report URL',v=>v.device_evidence.elite.url='https://malicious.test/not-owner');
rejects('short source commit',v=>v.source_commit='bad');
rejects('superseded rollback without approval',v=>v.lifecycle='superseded');
rejects('release checked not owner dev',v=>v.status='release-checked');
rejects('unreviewed Releases import',v=>v.models.elite.fromRelease=true);
rejects('offsite package',v=>v.models.elite.pkg='https://example.org/test.fwsc');
rejects('path traversal',v=>v.models.elite.pkg='../firmware/../x.fwsc');
rejects('zero size',v=>v.models.elite.size=0);
rejects('missing owner device report digest',v=>delete v.device_evidence.elite.sha256);
assert.deepEqual(evaluated(good,{[`${VER}/elite`]:{...approval,owner_publication_approved:false}}),[], 'static approval without publication flag');
console.log('PASS reject static record not owner approved');
console.log('PASS full checked source gate: 20 cases');
