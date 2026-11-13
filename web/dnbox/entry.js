import { PINNED, validateCatalog, runtimeCapabilities, audioPolicy } from './identity.mjs';
import { mountControls } from './controls.js';

// No synthetic audio or firmware bootstrap: run the existing fm1-web main.js only after
// checking its documented dependencies and matching the published DnBOX model/bytes.
const status = document.getElementById('status');
const start = document.getElementById('start');
const errorBox = document.getElementById('demo-error');
start.disabled = true;
async function ready() {
  const caps = runtimeCapabilities(globalThis);
  if (!caps.ok) throw new Error('missing ' + caps.missing.join(', '));
  const url = new URL(location.href);
  const policy = audioPolicy(url.searchParams);
  // A new visitor gets the true SMK-37 360 MHz CPU and a bounded 512-frame queue.
  if (!url.searchParams.has('mhz')) url.searchParams.set('mhz', String(policy.mhz));
  if (!url.searchParams.has('ahead')) url.searchParams.set('ahead', String(policy.ahead));
  if (!url.searchParams.has('latency')) url.searchParams.set('latency', policy.latency);
  // Never let a query parameter bypass the pinned firmware catalog or inject a
  // foreign try-out script/flash image into this public page.
  for (const key of ['package', 'name', 'flash', 'script']) url.searchParams.delete(key);
  const chosen = url.searchParams.get('fw');
  if (!Object.hasOwn(PINNED, chosen)) url.searchParams.set('fw', 'dnbox-elite-0251');
  history.replaceState(null, '', url.pathname + url.search + url.hash);
  const response = await fetch('firmwares.json', { cache: 'no-store' });
  if (!response.ok) throw new Error('firmware list HTTP ' + response.status);
  validateCatalog(await response.json());
  const controls = mountControls();
  try {
    // main.js is copied unchanged into /dnbox/ by build.sh except the shared optional
    // identity and control hooks. Its wasm, worklet and front panel are the real fm1-emu.
    await import('./main.js');
  } catch (err) {
    controls.releaseAll();
    throw err;
  }
  start.disabled = false;
  document.documentElement.dataset.ready = 'true';
  status.textContent = 'Click Enable sound & boot firmware; 360 MHz, ' +
    policy.queueMs.toFixed(1) + ' ms queue allowance (not measured output latency)';
}
try {
  await ready();
} catch (error) {
  document.documentElement.dataset.ready = 'error';
  start.disabled = true;
  start.hidden = true;
  status.textContent = 'DnBOX demo unavailable: ' + (error?.message || String(error));
  errorBox.hidden = false;
  console.error('DnBOX public emulator prerequisite failed:', error);
}
