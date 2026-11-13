/* Run from the repository root: node v2-preview/tests/flow-contract.mjs
 * Static preview contract only. NOT a hardware, permission, or installer execution test. */
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
const html = readFileSync(new URL('../install.html', import.meta.url), 'utf8');
const js = readFileSync(new URL('../install.js', import.meta.url), 'utf8');
const pageIds = new Set([...html.matchAll(/data-s="([a-z0-9-]+)"/g)].map(m => m[1]));
const targets = [...html.matchAll(/data-next="([a-z0-9-]+)"/g)].map(m => m[1]);
const stages = [...html.matchAll(/data-stages="([a-z0-9-]+)"/g)].map(m => m[1]);
assert.equal(pageIds.size, [...html.matchAll(/data-s="([a-z0-9-]+)"/g)].length, 'unique states');
for (const target of targets) assert(pageIds.has(target), 'missing transition target ' + target);
for (const stage of stages) assert(pageIds.has(stage) || stage === 'boot', 'missing stage ' + stage);
for (const required of ['permission','path-unverified','err-midi-denied','err-midi-unavailable','err-midi-device','backup-warning','transfer','restart','boot','err-nostart']) {
  assert(pageIds.has(required), 'missing required state ' + required);
}
for (const bad of ['data-next="done"','data-s="done"','data-need="bak"','Both of these are supported','physical startup is not verified','Not verified on hardware']) {
  assert(!html.includes(bad), 'obsolete install claim/gate: ' + bad);
}
assert(html.includes('WARNING · not a block') || html.includes('WARNING · not a block'.replace(' · ', ' · ')), 'backup warning');
assert(js.includes('chosen("bak").value === "done" ? "writing" : "backup-warning"'), 'warning only on failed/skipped backup');
assert(js.includes('chosen("path").value === "iphone" ? "path-unverified" : "permission"'), 'unverified route isolated');
assert(html.indexOf('data-s="transfer"') < html.indexOf('data-s="restart"') && html.indexOf('data-s="restart"') < html.indexOf('data-s="boot"'), 'distinct post-write stages');
assert(html.includes('exact installed package hash') || html.includes('installed package hash'), 'owner evidence limitation');
assert(html.includes('no Pro') || html.includes('Pro installation'), 'model test limitation');
assert(!/requestMIDIAccess|navigator\.usb|navigator\.serial|WebUSB|fetch\(|XMLHttpRequest|new WebSocket|postMessage\(/i.test(js), 'no device or network API');
assert(!/data-device-write|writeFirmware|sendSysex/i.test(html+js), 'no real firmware actions');
console.log('PASS: unique preview states=' + pageIds.size + ', routed links=' + targets.length +
  ', visible stages=' + stages.length + '; negative device/writes and evidence checks PASS.');
