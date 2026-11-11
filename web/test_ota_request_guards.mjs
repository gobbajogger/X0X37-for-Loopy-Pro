// SPDX-License-Identifier: GPL-3.0-only
// OTA read-request framing, terminal/zero-length guards and the bounded receive queue.
// Ported from the archived installer lane (EC027, EC028, EC032). No MIDI device, no writes.
// Run: node web/test_ota_request_guards.mjs
import assert from "node:assert/strict";
import { Updater, pack7 } from "./fm1ota.js";

const req = (addr, len, fl = 0, body8 = 8) => {
  const b = [0x00, 0x59, 0x30, body8, 0, 0, fl, addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF,
    len & 0xFF, (len >> 8) & 0xFF, (len >> 16) & 0xFF];
  let s = 0; for (let i = 6; i < 14; i++) s += b[i];
  b.push(~s & 0xFF);
  return [0xF0, ...pack7(b), 0xF7];
};
function mockLink(frames, { sendOk = true } = {}) {
  const sent = [];
  return { sent, lost: false, async read() { return frames.length ? frames.shift() : null; },
    send(x) { sent.push(x); return sendOk; } };
}
const up = Object.create(Updater.prototype);
const image = new Uint8Array(4096);

// Ordinary read then app terminal read: served and finished.
{
  const l = mockLink([req(0, 16), req(0xE0000000, 8)]);
  const r = await up.serve(l, image, 0xE0000000, 50);
  assert.deepEqual(r, { served: 1, finished: true });
  assert.equal(l.sent.length, 2);
}
// Zero-length ordinary read is rejected.
await assert.rejects(up.serve(mockLink([req(0, 0)]), image, 0xE0000000, 50), (e) => e.code === "badreq");
// A terminal read that is not the eight-byte success read is rejected.
await assert.rejects(up.serve(mockLink([req(0xF0000000, 0)]), image, 0xF0000000, 50), (e) => e.code === "badreq");
await assert.rejects(up.serve(mockLink([req(0xE0000000, 512)]), image, 0xE0000000, 50), (e) => e.code === "badreq");
assert.deepEqual(await up.serve(mockLink([req(0xF0000000, 8)]), image, 0xF0000000, 50), { served: 0, finished: true });
// A terminal reply that cannot be sent is a lost link, not a finish.
{
  const r = await up.serve(mockLink([req(0xE0000000, 8)], { sendOk: false }), image, 0xE0000000, 50);
  assert.deepEqual(r, { served: 0, finished: false, lost: true });
}
// Wrong declared body length or non-canonical padding: ignored (never answered).
{
  const bad = req(0, 16, 0, 9);
  const pad = req(0, 16); pad[pad.length - 2] |= 0x40;   // set unused high padding bits
  const hi = req(0, 16); hi[3] |= 0x80;                   // status byte inside SysEx
  const l = mockLink([bad, pad, hi]);
  const r = await up.serve(l, image, 0xE0000000, 0);
  assert.equal(r.finished, false); assert.equal(l.sent.length, 0);
}

// Bounded receive queue: overflow poisons the link and read() throws "overflow".
{
  const src = await import("node:fs").then((fs) => fs.readFileSync(new URL("./fm1ota.js", import.meta.url), "utf8"));
  assert.match(src, /this\.q\.length >= 64 \|\| ev\.data\.length > 65536 - this\.rxBytes/);
  const mod = await import("./fm1ota.js");
  const input = { state: "connected", addEventListener(_, f) { this.f = f; }, removeEventListener() {} };
  const output = { state: "connected", send() {} };
  const LinkCtor = mod.__test_Link;
  if (LinkCtor) {
    const link = new LinkCtor(input, output);
    for (let i = 0; i < 64; i++) input.f({ data: [0xF0, 1, 0xF7] });
    assert.equal(link.q.length, 64);
    input.f({ data: [0xF0, 1, 0xF7] });
    await assert.rejects(link.read(5), (e) => e.code === "overflow");
    input.f({ data: [0xF0, 2, 0xF7] });                   // further input stays dropped
    await assert.rejects(link.read(5), (e) => e.code === "overflow");
    const l2 = new LinkCtor(input, output);
    input.f({ data: new Array(65537).fill(0) .map((_, i) => i === 0 ? 0xF0 : 0) });
    await assert.rejects(l2.read(5), (e) => e.code === "overflow");
    const l3 = new LinkCtor(input, output);
    input.f({ data: [0xF0, 3, 0xF7] });
    assert.deepEqual(await l3.read(5), [0xF0, 3, 0xF7]);
    assert.equal(l3.rxBytes, 0);
  } else assert.fail("Link test export absent");
}
console.log("ota request guards: ok");
