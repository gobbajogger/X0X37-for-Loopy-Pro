// SPDX-License-Identifier: GPL-3.0-only
// Real installer source-function admission tests; synthetic data only, no downloads or device.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { Script } from 'node:vm';
const source=readFileSync(fileURLToPath(new URL('../web/x0x_installer.html',import.meta.url)),'utf8');
assert.doesNotMatch(source,/mergeGitHubReleases|api\.github\.com\/repos\//,
  'no unapproved GitHub Release inventory may reach NIGHTLY');
const span=(first,next)=>{
  const start=source.indexOf(first);
  const end=source.indexOf(next,start);
  assert.ok(start>=0 && end>start,'required installer function missing: '+first);
  return source.slice(start,end);
};
const code=[
  span('function modelKeyForProduct(product) {','\nfunction availableModels'),
  span('function refreshMeta() {','\nfunction isCatalogChecked'),
  span('function isCatalogChecked(v,p) {','\nfunction updateInstallGate')
].join('\n');
const download={
  hidden:false,href:'https://stale.invalid/firmware.fwsc',download:'',
  removeAttribute(name){if(name==='href')delete this.href;}
};
const elems={pkgmeta:{textContent:''},download,builddetail:{hidden:false}};
const context={
  chosenModel:'pro',currentVersion:()=>null,
  $:name=>elems[name],renderBuildInfo:()=>{},updateInstallGate:()=>{}
};
new Script(code+'\nthis.refreshMeta = refreshMeta;', {filename:'installer-source-functions.js'}).runInNewContext(context);
function check(version,expectedHref) {
  context.currentVersion=()=>version;
  context.refreshMeta();
  assert.equal(download.hidden,expectedHref===null,'download visibility mismatch');
  assert.equal(download.href??null,expectedHref,'download href must be cleared when unapproved');
}
const model=p=>({status:'dev-checked',version:'20261024',models:{pro:p}});
const good={product:'SMK-37 Pro_900',pkg:'https://example.org/checked.fwsc',sha256:'a'.repeat(64),size:651392};
check(null,null);
check(model(good),good.pkg);
check(model({...good,fromRelease:true,pkg:'https://github.com/unreviewed/release.fwsc'}),null);
check(model({...good,product:'SMK-37 Elite_900'}),null);
check(model({...good,pkg:'http://example.org/unverified.fwsc'}),null);
check(model({...good,sha256:''}),null);
check(model({...good,sha256:'0'.repeat(63)}),null);
check(null,null);
console.log('PASS: no GitHub release import, approved model-link projection and stale-href clearing');
