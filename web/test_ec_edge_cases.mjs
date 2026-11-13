// SPDX-License-Identifier: GPL-3.0-only
// EC028/EC032 (OTA serve), EC045 (bound port statechange), EC046 (bounded import preflight).
import test from 'node:test';
import assert from 'node:assert/strict';
import { Updater, pack7 } from './fm1ota.js';
import { watchDeviceSession } from './device_session.js';
import { importResampledLength, cutLoop, resample, SLOT_DATA_OFF } from './x0x_breaks.js';

function request(addr, len) {
  const u = [0x00, 0x59, 0x30, 0, 0, 0, 0, addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF,
    len & 0xFF, (len >> 8) & 0xFF, (len >> 16) & 0xFF];
  let s = 0; for (let i = 6; i < 14; i++) s += u[i];
  u.push(~s & 0xFF);
  return [0xF0, ...pack7(u), 0xF7];
}
const linkOf = (reqs) => ({ sent: [], read() { return Promise.resolve(reqs.shift() ?? null); }, send(f) { this.sent.push(f); return true; } });
const image = new Uint8Array(1024);

test('EC028: the finish read must ask for the eight-byte success token', async () => {
  const up = new Updater({});
  assert.deepEqual(await up.serve(linkOf([request(0xF0000000, 8)]), image, 0xF0000000, 50), { served: 0, finished: true });
  for (const len of [0, 7, 9])
    await assert.rejects(up.serve(linkOf([request(0xE0000000, len)]), image, 0xE0000000, 50), (e) => e.code === 'badreq');
});

test('EC032: a zero-length data read is a bad request', async () => {
  const up = new Updater({});
  await assert.rejects(up.serve(linkOf([request(0, 0)]), image, 0xF0000000, 50), (e) => e.code === 'badreq');
});

function access(input, output) {
  const l = [];
  return { inputs: new Map([[input.id, input]]), outputs: new Map([[output.id, output]]),
    addEventListener(_, f) { l.push(f); }, removeEventListener() {}, fire(port) { l.forEach((f) => f({ port })); } };
}

test('EC045: once armed, any statechange of a bound port invalidates the session', () => {
  const input = { id: 'i', type: 'input', state: 'connected' }, output = { id: 'o', type: 'output', state: 'connected' };
  let armed = false, hits = 0;
  const a = access(input, output);
  const s = watchDeviceSession(a, input, output, () => hits++, () => armed);
  a.fire(input); assert.equal(hits, 0); // implicit-open event before identification
  armed = true;
  a.fire({ id: 'x', type: 'input' }); assert.equal(hits, 0); // unrelated port
  a.fire({ id: 'o', type: 'output', state: 'connected' }); assert.equal(hits, 1); // reconnected, same id
  assert.throws(() => s.assert());
  const legacy = watchDeviceSession(a, input, output, () => hits++);
  a.fire(input); assert.equal(hits, 1); legacy.assert(); // no armed(): old behavior
});

test('EC046: preflight length equals resample() and rejects before allocation', () => {
  for (const [n, sr, to] of [[1000, 44100, 32000], [777, 48000, 22050], [10, 8000, 32000], [500, 32000, 32000]])
    assert.equal(importResampledLength(n, sr, to, 1e9), resample(new Array(n).fill(0), sr, to).length);
  assert.throws(() => importResampledLength(1e12, 44100, 32000, 1000), /capacity/);
  const x = Array.from({ length: 48000 }, (_, i) => Math.sin(i / 7));
  assert.throws(() => cutLoop(x, 48000, false, 32000, false, 1, 1000), /capacity/);
  assert.ok(cutLoop(x, 48000, false, 32000, false, 1, 40000 - SLOT_DATA_OFF).s.length <= 2 * (40000 - SLOT_DATA_OFF));
  assert.throws(() => cutLoop(x, 48000, false, 32000, false, 1, 0), /Invalid/);
});
