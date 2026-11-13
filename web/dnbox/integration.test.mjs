import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
const from = path => readFileSync(new URL(path, import.meta.url), 'utf8');
const html = from('./index.html');
const entry = from('./entry.js');
const page = from('../main.js');
const worklet = from('../worklet.js');
const build = from('../build.sh');

test('all actual fm1-emu main.js DOM targets exist on the DnBOX page', () => {
  const ids = [...page.matchAll(/getElementById\(["']([^"']+)/g)].map(m => m[1]);
  assert(ids.length > 20, 'missing upstream UI dependency inventory');
  for (const id of ids) assert(html.includes('id="' + id + '"'), id);
});
test('dedicated build bundles real wasm, panel, worklet and controls without firmware binaries', () => {
  assert.match(build, /mkdir -p "\$OUT\/dnbox"/);
  for (const file of ['fm1_web.wasm','worklet.js','main.js','jit.js','persistence.js',
    'tryout.js','fwsc.js','entry.js','identity.mjs','controls.js','firmwares.json']) {
    assert(build.includes(file), 'missing build asset ' + file);
  }
  assert(!build.includes('dnbox-elite-dev.fwsc') && !build.includes('dnbox-pro-dev.fwsc'));
  assert.match(entry, /await import\('\.\/main\.js'\)/);
});
test('page does not request installation access, and user gesture is required for audio', () => {
  for (const source of [html,entry,from('./controls.js')]) {
    assert.doesNotMatch(source, /requestDevice\(|requestPort\(|navigator\.usb|navigator\.serial|navigator\.hid/i);
  }
  assert.match(page, /startBtn\.addEventListener\("click", \(\) => start\(\)\)/);
  assert.match(page, /new AudioContext\(/);
  assert.match(page, /await ac\.resume\(\)/);
  assert.match(entry, /'package', 'name', 'flash', 'script'/);
});
test('accessible controls feed physical GPIO/ADC and receive boot lifecycle, not a synth', () => {
  assert.match(page, /dnbox:input/);
  assert.match(page, /dnbox:booting/);
  assert.match(page, /dnbox:booted/);
  assert.match(page, /dnbox:stopped/);
  assert.match(page, /setAnalog\(id, on \? 700 : 0\)/);
  assert.match(page, /if \(on\) press\(id\); else unpress\(id\);/);
  assert.doesNotMatch(from('./controls.js'), /OscillatorNode|createOscillator|AudioBufferSourceNode/);
});
test('sampled input round-trip and audio shortfall are measured, not acoustic claims', () => {
  assert.match(page, /input-ack/);
  assert.match(page, /dnbox:telemetry/);
  assert.match(worklet, /type: "input-ack"/);
  assert.match(worklet, /short: this\.short/);
  assert.match(html, /not measured key-to-speaker latency/i);
  assert.match(html, /not physical-device performance/i);
  assert.match(entry, /Input message round trip/);
});
