// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import { LABELS, availableMidi, visibleVersionHistory } from './dnbox_site.js';

for (const locale of ['en', 'ru', 'ja']) {
  for (const key of ['install', 'samples', 'projects', 'manual', 'history', 'browserNo', 'fileInvalid', 'preview']) {
    assert.equal(typeof LABELS[locale][key], 'string', `${locale}/${key} present`);
    assert.ok(LABELS[locale][key].length > 1, `${locale}/${key} readable`);
  }
}
assert.equal(availableMidi({isSecureContext:false,navigator:{requestMIDIAccess(){}}}),'insecure');
assert.equal(availableMidi({isSecureContext:true,navigator:{}}),'unsupported');
assert.equal(availableMidi({isSecureContext:true,navigator:{requestMIDIAccess(){}}}),'supported');
assert.equal(availableMidi({isSecureContext:true,navigator:{requestMIDIAccess:1}}),'unsupported');
assert.equal(availableMidi({isSecureContext:true,navigator:{requestMIDIAccess(){throw Error('denied')}}}),'supported',
  'support does not imply permission granted');
const hash='a'.repeat(64), bad='b'.repeat(63);
const catalogue={versions:[{version:'0.0.7',models:{pro:{sha256:hash,file:'pro.fwsc'},elite:{sha256:bad,file:'elite.fwsc'}}}]};
const rows=visibleVersionHistory(catalogue,p=>p.file==='pro.fwsc');
assert.equal(rows.length,1);
assert.equal(rows[0].version,'0.0.7','number taken only from catalog');
assert.equal(rows[0].models[0].available,true);
assert.equal(rows[0].models[1].available,false);
assert.equal(rows[0].models[1].digest,'');
assert.deepEqual(visibleVersionHistory({versions:[]}),[]);
assert.deepEqual(visibleVersionHistory({}),[]);
console.log('PASS: 29 locale, capability, and catalog assertions; no hardware claims');