// DnBOX-specific public-demo contract. The actual CPU, LCD, MIDI and audio remain in fm1-web.
// This file validates a pinned catalog and the browser route; it never synthesizes sound.
export const RELEASE = '0.0.25.1';
export const SOURCE = '79280e763341d03ba43266d3e25dad66f12de860';
export const PINNED = Object.freeze({
  'dnbox-elite-0251': Object.freeze({ model: 'SMK-37 Elite_900', sha256: '5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a' }),
  'dnbox-pro-0251': Object.freeze({ model: 'SMK-37 Pro_900', sha256: '6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf' }),
});
const ALLOWED_AHEAD = new Set([256, 512, 1024]);
const ALLOWED_CLOCKS = new Set([60, 80, 100, 120, 160, 200, 240, 360]);

export function validateCatalog(catalog) {
  if (!catalog || !Array.isArray(catalog.firmwares) || catalog.firmwares.length !== 2 ||
      catalog.default !== 'dnbox-elite-0251') throw new Error('unexpected DnBOX firmware catalog');
  const ids = new Set();
  for (const f of catalog.firmwares) {
    const pin = PINNED[f.id];
    if (!pin || ids.has(f.id) || f.model !== pin.model || f.sha256 !== pin.sha256 ||
        f.version !== RELEASE || f.source_commit !== SOURCE || f.size !== 651721 ||
        f.name !== ('DnBOX · ' + (f.id === 'dnbox-elite-0251' ? 'Elite' : 'Pro'))) {
      throw new Error('DnBOX model, identity or package provenance mismatch');
    }
    let url;
    try { url = new URL(f.package); } catch { throw new Error('invalid firmware package URL'); }
    if (url.protocol !== 'https:' || url.hostname !== 'gobbajogger.github.io' ||
        !url.pathname.startsWith('/x0x37-for-Loopy-Pro_nightly/artifacts/nightly-dev/' + f.sha256 + '/') ||
        !url.pathname.endsWith(f.id === 'dnbox-elite-0251' ? '/x0x37-elite-dev.fwsc' : '/x0x37-pro-dev.fwsc') ||
        url.search || url.hash) throw new Error('untrusted DnBOX package location');
    ids.add(f.id);
  }
  return catalog;
}

export function runtimeCapabilities(root) {
  const missing = [];
  if (!root.isSecureContext) missing.push('HTTPS or localhost');
  if (!root.WebAssembly?.instantiate) missing.push('WebAssembly');
  if (typeof root.AudioContext !== 'function') missing.push('Web Audio');
  if (typeof root.AudioWorkletNode !== 'function') missing.push('AudioWorklet');
  if (typeof root.ResizeObserver !== 'function') missing.push('ResizeObserver');
  if (!root.crypto?.subtle?.digest) missing.push('SHA-256 verification');
  return { ok: missing.length === 0, missing };
}

// 512 queued frames = 11.61ms at 44.1kHz. This is a queue allowance, NOT
// measured end-to-end key-to-sound latency. 2.5-second masking is not offered.
export function audioPolicy(params) {
  const ahead = params.has('ahead') ? Number(params.get('ahead')) : 512;
  const mhz = params.has('mhz') ? Number(params.get('mhz')) : 360;
  const latency = params.get('latency') ?? 'interactive';
  if (!ALLOWED_AHEAD.has(ahead) || !ALLOWED_CLOCKS.has(mhz) ||
      !['interactive', 'balanced', 'playback'].includes(latency)) {
    throw new Error('invalid clock or audio-buffer setting');
  }
  return { ahead, mhz, latency, queueMs: ahead * 1000 / 44100 };
}
