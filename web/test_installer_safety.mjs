// SPDX-License-Identifier: GPL-3.0-only
// Reboot target binding, failed-layout preservation and exact sample headers.
// Mocked MIDI/NOR only: node web/test_installer_safety.mjs
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Updater, pack7, unpack7, sameKeyboard } from "./fm1ota.js";
import { installKeepingSlots, userSlotRaw, writeSlotImage, slotHeaderForWrite, pack78, unpack78, parseInfo } from "./x0x_breaks.js";

const nativeTimeout = globalThis.setTimeout;
const scale = Number(process.env.X0X_TEST_TIME_SCALE || 20);
globalThis.setTimeout = (f, ms = 0, ...a) => nativeTimeout(f, ms >= 1000 ? ms / scale : ms, ...a);
const HS = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];
const same = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
let passed = 0;
async function test(name, fn) { await fn(); console.log(`ok ${++passed} ${name}`); }

class Keyboard {
  constructor(model = "Pro", loader = false, name = "SMK-37 TARGET") {
    this.product = `SMK-37 ${model}_900`;
    this.nextLoader = `ota-${this.product}`;
    this.upgrades = 0;
    this.layout = null;
    this.boot(loader ? this.nextLoader : `SMK-37 ${model}_016`, name);
  }
  boot(identity, name) {
    if (this.input) this.input.state = this.output.state = "disconnected";
    this.identity = identity;
    this.input = { name, state: "connected", open: async () => {}, onmidimessage: null };
    this.output = { name, state: "connected", open: async () => {}, send: (d) => this.rx(d) };
  }
  tx(data) { this.input.onmidimessage?.({ data }); }
  request(addr) {
    const u = [0, 0x59, 0x30, 0, 0, 0, 0, addr & 255, addr >>> 8 & 255, addr >>> 16 & 255, addr >>> 24, 8, 0, 0];
    u.push(~u.slice(6).reduce((a, b) => a + b, 0) & 255);
    this.tx([0xF0, ...pack7(u), 0xF7]);
  }
  rx(d) {
    if (same(d, HS)) {
      const body = [0, 0x59, 0x11, 0, 0, 0, ...new TextEncoder().encode(this.identity)];
      while (body.length < 34) body.push(0);
      this.tx([0xF0, ...pack7(body), 0xF7]);
    } else if (same(d, UPGRADE)) {
      this.upgrades++;
      this.finish = this.identity.startsWith("ota-") ? 0xF0000000 : 0xE0000000;
      this.request(this.finish);
    } else if (d[1] === 0x7D && d[4] === 15) {
      if (this.layout !== null) this.tx([0xF0, 0x7D, 0x46, 0x4C, 15, ...this.layout, 0xF7]);
    } else {
      const u = unpack7(d.slice(1, -1));
      const addr = (u[7] | u[8] << 8 | u[9] << 16 | u[10] << 24) >>> 0;
      if (addr === 0xE0000000) setTimeout(() => this.boot(this.nextLoader, "Felucca Update"), 5);
      if (addr === 0xF0000000) setTimeout(() => this.boot(this.product, "Felucca"), 5);
    }
  }
}

function access(devices) {
  return { get inputs() { return new Map(devices.map((d, i) => [i, d.input])); },
    get outputs() { return new Map(devices.map((d, i) => [i, d.output])); } };
}
const image = new Uint8Array(1024);
for (const model of ["Pro", "Elite"]) {
  await test(`${model}: two keyboards, both enumeration orders and loader/running bystanders`, async () => {
    for (const loader of [false, true]) for (const reversed of [false, true]) {
      const target = new Keyboard(model), other = new Keyboard(model, loader, "Felucca OTHER");
      const up = new Updater(access(reversed ? [other, target] : [target, other]));
      await assert.rejects(up.install(image, target.product), { code: "ambiguous" });
      assert.equal(target.upgrades + other.upgrades, 0);
    }
  });
  await test(`${model}: one keyboard completes across changed port names`, async () => {
    const target = new Keyboard(model), up = new Updater(access([target]));
    assert.equal(await up.install(image, target.product), target.product);
    assert.equal(target.upgrades, 2);
  });
}
await test("duplicate port names are ambiguous", async () => {
  const a = new Keyboard(), b = new Keyboard();
  await assert.rejects(new Updater(access([a, b])).install(image, a.product), { code: "ambiguous" });
  assert.equal(a.upgrades + b.upgrades, 0);
});
await test("FM-1 reference keeps its own model through the loader", async () => {
  const target = new Keyboard();
  target.product = "FM-1_900";
  target.nextLoader = "ota-FM-1_900";
  target.boot("FM-1_015", "FM-1");
  assert.equal(await new Updater(access([target])).install(image, target.product), target.product);
  assert.equal(target.upgrades, 2);
});
await test("resume rejects an incompatible package before writing", async () => {
  const target = new Keyboard("Elite", true);
  await assert.rejects(new Updater(access([target])).resume(image, null, "FM-1_900"), { code: "model" });
  assert.equal(target.upgrades, 0);
});
await test("bystander appearing during reboot receives no write", async () => {
  const target = new Keyboard(), other = new Keyboard("Pro", true, "Felucca OTHER"), devices = [target];
  await assert.rejects(new Updater(access(devices)).install(image, target.product, (phase) => {
    if (phase === "loader") devices.push(other);
  }), { code: "ambiguous" });
  assert.equal(target.upgrades, 1);
  assert.equal(other.upgrades, 0);
});
await test("loader model is checked before the write command", async () => {
  const target = new Keyboard();
  target.nextLoader = "ota-FM-1_900";
  await assert.rejects(new Updater(access([target])).install(image, target.product), { code: "model" });
  assert.equal(target.upgrades, 1);
});
await test("verified target name remains bound for sample restore", async () => {
  const target = new Keyboard(), up = new Updater(access([target]));
  await up.install(image, target.product);
  target.boot(target.product, "Felucca OTHER");
  await assert.rejects(up.waitFor(() => true, 100), { code: "ambiguous" });
  assert.equal(target.upgrades, 2);
});
await test("unknown, empty and malformed sample layouts stop before update", async () => {
  for (const reply of [null, [], [0, 44], [3, 44]]) {
    const target = new Keyboard();
    target.boot(target.product, "SMK-37 TARGET");
    target.layout = reply;
    await assert.rejects(installKeepingSlots(new Updater(access([target])), image, target.product, "smk", { sameKeyboard }), { code: "slotbackup" });
    assert.equal(target.upgrades, 0);
  }
});
await test("documented stock identity can install without sample commands", async () => {
  const target = new Keyboard();
  const r = await installKeepingSlots(new Updater(access([target])), image, target.product, "smk", { sameKeyboard });
  assert.equal(r.product, target.product);
  assert.equal(target.upgrades, 2);
});

class SlotLink {
  constructor(full = true) { this.full = full; this.flash = new Uint8Array(4096).fill(255); this.queue = []; this.commands = []; this.headers = []; }
  drain() { this.queue = []; }
  async read() { return this.queue.shift() || null; }
  async req(cmd, a) { return this.handle(cmd, a); }
  async retry(cmd, a) { return this.handle(cmd, a); }
  send(f) {
    const cmd = f[4], r = this.handle(cmd, f.slice(5, -1));
    this.queue.push([0xF0, 0x7D, 0x46, 0x4C, cmd, ...r, 0xF7]);
    return true;
  }
  handle(cmd, a) {
    this.commands.push(cmd);
    if (cmd === 9 && a[0] === 64) {
      if (!this.full) throw Error("legacy firmware does not support QUERY");
      return [64, 0, 1, 64, 0, 1, 0];
    }
    if (cmd === 11) { this.flash.fill(255); return [a[0], 0]; }
    if (cmd === 12) { this.flash.set(unpack78(a.slice(4)), a[1] | a[2] << 7 | a[3] << 14); return [...a.slice(0, 4), 0]; }
    if (cmd === 13) {
      const h = unpack78(a.slice(1));
      this.headers.push(h.length);
      if (h.length !== 480 && !(this.full && h.length === 512)) return [a[0], 1];
      this.flash.set(h);
      return [a[0], 0];
    }
    throw Error(`Unexpected slot command ${cmd}`);
  }
}
const slot = userSlotRaw("SAFE", [{ adpcm: new Uint8Array(32).fill(7), n: 64, rate: 44100 }], 4096);
const exact = new Uint8Array(544).fill(255);
exact.set(slot.hdr); exact.set(slot.data, 512); exact.set(Uint8Array.from({ length: 32 }, (_, i) => i), 480);
const legacy = exact.slice(); legacy.fill(255, 480, 512);
const html = readFileSync(new URL("x0x_breaks.html", import.meta.url), "utf8");
const pageWriter = /async function writeImage\(k, img, onPart\) \{[\s\S]*?\n\}\n/.exec(html)[0];
for (const writer of ["library", "page"]) {
  const write = (link, img) => writer === "library" ? writeSlotImage(link, 0, img) :
    new Function("slotHeaderForWrite", "SLOT_DATA_OFF", "pack78", "link", `return (${pageWriter});`)(slotHeaderForWrite, 512, pack78, link)(0, img);
  await test(`${writer}: complete header and audio bytes restored exactly`, async () => {
    const link = new SlotLink(); await write(link, exact);
    assert.deepEqual(link.flash.slice(0, exact.length), exact);
    assert.deepEqual(link.headers, [512]);
  });
  await test(`${writer}: erased header tail retains legacy firmware compatibility`, async () => {
    const link = new SlotLink(false); await write(link, legacy);
    assert.deepEqual(link.flash.slice(0, legacy.length), legacy);
    assert.deepEqual(link.headers, [480]);
    assert(!link.commands.includes(9));
  });
  await test(`${writer}: unsupported header tail is rejected before erasure`, async () => {
    const link = new SlotLink(false); link.flash.fill(21);
    const before = link.flash.slice();
    await assert.rejects(write(link, exact), /nothing was erased/);
    assert.deepEqual(link.flash, before);
    assert.deepEqual(link.commands, [9]);
  });
}
await test("malformed slot layouts are rejected", async () => {
  for (const r of [[0, 80], [2, 0], [1, 80, 0], [1, 80, 0, 0, 0, 88, 1]]) assert.throws(() => parseInfo(r));
});
console.log(`${passed} installer/sample safety regressions passed`);
