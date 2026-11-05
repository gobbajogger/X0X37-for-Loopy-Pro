// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
//
// Node checks of the web package / OTA helpers (no browser, no hardware). Run from the repo root:
//   node web/test_web.mjs
// The Felucca web editor (web/editor.html) and its protocol harness were retired with the X0X
// fork (commit 23775375); do not restore them here. Remaining checks:
// - fm1pkg.js: productOf and logicalImage on build/felucca.fwsc (skipped without a build)
// - fm1ota.js: a full install and an unplug during the write against a simulated FM-1

import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";
import { execFileSync } from "node:child_process";
import { logicalImage, productOf } from "./fm1pkg.js";
import { Updater, pack7, unpack7 } from "./fm1ota.js";
import { parseInfo, installKeepingSlots } from "./x0x_breaks.js";

let failed = 0;
const ok = (cond, what) => { console.log(`${what.padEnd(64)} ${cond ? "ok" : "FAIL"}`); if (!cond) failed++; };
const eq = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
const py = (code, ...args) => execFileSync("python3", ["-c", code, ...args], { maxBuffer: 1 << 26 });
const HERE = new URL(".", import.meta.url).pathname;

/* ------------------------------------------------------- packages: JS == Python --- */
async function packages() {
  const pkg = join(HERE, "../build/felucca.fwsc");
  if (!existsSync(pkg)) {
    console.log("packages: skipped (run ./build.sh first)");
    return;
  }
  const raw = readFileSync(pkg);
  const logical = py(`import sys; raw = open(sys.argv[1], "rb").read()
sys.stdout.buffer.write(b"".join(raw[i * 48:i * 48 + 47] for i in range(20)) + raw[960:])`, pkg);
  ok(eq(logicalImage(raw), logical), "fm1pkg.js logicalImage");
  ok(/^FM-1_9\d\d$/.test(productOf(raw)), "fm1pkg.js productOf");
}

/* ------------------------------------------------- update protocol (fm1ota.js) --- */
const HS = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];

/* an FM-1 on WebMIDI: identity, then "device asks, host answers" reads of the image */
class FakeFM1 {
  constructor(image, { unplugAfter = Infinity } = {}) {
    this.image = image; this.unplugAfter = unplugAfter; this.served = 0; this.bad = 0;
    this.access = { inputs: new Map(), outputs: new Map() };
    this.boot("FM-1_015", "FM-1");
  }
  boot(identity, name) {
    this.identity = identity; this.waiting = null; this.queue = [];
    for (const m of [this.access.inputs, this.access.outputs]) { for (const p of m.values()) p.state = "disconnected"; m.clear(); }
    const id = Math.random().toString(36).slice(2);
    this.input = { id: "i" + id, name, state: "connected", onmidimessage: null, open: async () => {} };
    this.output = { id: "o" + id, name, state: "connected", open: async () => {}, send: (d) => {
      if (this.output.state !== "connected") throw new Error("InvalidStateError");
      setTimeout(() => this.rx(Array.from(d)), 1);
    } };
    this.access.inputs.set(this.input.id, this.input);
    this.access.outputs.set(this.output.id, this.output);
  }
  tx(bytes) { const i = this.input; setTimeout(() => { if (i.state === "connected" && i.onmidimessage) i.onmidimessage({ data: Uint8Array.from(bytes) }); }, 1); }
  rx(d) {
    if (eq(d, HS)) {
      const t = [...new TextEncoder().encode(this.identity)];
      const payload = [...t, ...new Array(27 - t.length).fill(0)];
      const body = [0, 0x59, 0x11, 27, 0, 0, ...payload, ~payload.reduce((sum, b) => sum + b, 0) & 255];
      this.tx([0xF0, ...pack7(body), 0xF7]);
    } else if (eq(d, UPGRADE)) {
      this.queue = this.identity.startsWith("ota-")
        ? [...Array.from({ length: 6 }, (_, k) => [k * 512, 512]), [0xF0000000, 8]]
        : [[0, 64], [0x40, 160], [0x1000, 512], [0xE0000000, 8]];
      this.next();
    } else if (this.waiting) {
      const u = unpack7(d.slice(1, -1));
      const [addr, len] = this.waiting;
      const got = u.slice(14, 14 + (addr >= 0xE0000000 ? 8 : len));
      const want = addr >= 0xE0000000 ? [...new TextEncoder().encode("success"), 0] : Array.from(this.image.subarray(addr, addr + len));
      if (!eq(got, want)) this.bad++;
      this.waiting = null;
      this.served++;
      if (this.served >= this.unplugAfter) { this.input.state = this.output.state = "disconnected"; return; }
      if (addr === 0xE0000000) setTimeout(() => this.boot("ota-FM-1_900", "Felucca Update"), 300);
      else if (addr === 0xF0000000) setTimeout(() => this.boot("FM-1_900", "Felucca"), 300);
      else this.next();
    }
  }
  next() {
    const r = this.queue.shift();
    if (!r) return;
    this.waiting = r;
    const [addr, len] = r;
    const u = [0, 0x59, 0x30, 0, 0, 0, 0, addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF, len & 0xFF, len >> 8, 0];
    let s = 0;
    for (let i = 6; i < 14; i++) s += u[i];
    u.push(~s & 0xFF);
    this.tx([0xF0, ...pack7(u), 0xF7]);
  }
}

async function updater() {
  const image = Uint8Array.from({ length: 0x2000 }, (_, i) => (i * 7) & 0xFF);
  const dev = new FakeFM1(image);
  const steps = [];
  const got = await new Updater(dev.access).install(image, "FM-1_900", (k) => steps.push(k));
  ok(got === "FM-1_900" && dev.bad === 0 && steps.includes("write") && steps.at(-1) === "done",
    `fm1ota.js: install: running firmware -> loader -> Felucca (${dev.served} reads)`);

  const dev2 = new FakeFM1(image, { unplugAfter: 3 });
  dev2.boot("ota-FM-1_900", "Felucca Update");
  const t0 = Date.now();
  const done = await new Updater(dev2.access).resume(image, null, "FM-1_900");
  ok(done === false && Date.now() - t0 < 6000, "fm1ota.js: unplugged during the write -> stops at once");

  const dev3 = new FakeFM1(image, { unplugAfter: 2 });
  const e = await new Updater(dev3.access).install(image, "FM-1_900").then(() => null, (x) => x);
  ok(e && e.code === "lost", "fm1ota.js: unplugged in step 1 -> error code 'lost'");
  const e2 = await new Updater({ inputs: new Map(), outputs: new Map() }).install(image, "FM-1_900").then(() => null, (x) => x);
  ok(e2 && e2.code === "notfound", "fm1ota.js: no device -> error code 'notfound'");
  /* resume() checks the loader's model against the package, as install() checks the running firmware */
  const smkOta = new FakeFM1(image);
  smkOta.boot("ota-SMK-37 Pro_900", "Felucca Update");
  const m1 = await new Updater(smkOta.access).resume(image, null, "FM-1_900").then(() => null, (x) => x);
  ok(m1 && m1.code === "model" && smkOta.served === 0, "fm1ota.js: an SMK-37 Pro in update mode is never given an FM-1 package ('model')");
  const noPkg = new FakeFM1(image);
  noPkg.boot("ota-FM-1_900", "Felucca Update");
  const m2 = await new Updater(noPkg.access).resume(image).then(() => null, (x) => x);
  ok(m2 && m2.code === "model" && noPkg.served === 0, "fm1ota.js: resume without the package's identity writes nothing");
  const elite = new FakeFM1(image);
  elite.boot("ota-SMK-37 Elite_900", "Felucca Update");
  const r3 = await new Updater(elite.access).resume(image, null, "SMK-37 Pro_900");
  ok(r3 === true && elite.served > 0, "fm1ota.js: resume: the SMK models are one keyboard, as for install()");
}


/* Corrupt SMP_INFO must not disguise occupied samples as empty before OTA. */
async function sampleInfoEmptyGuard() {
  const empty = [1,80,0,0,0], occupied = [1,80,1,76,79,79,80,0,1];
  const named = [1,80,0,76,79,79,80,0,1], used = [1,80,0,0,1];
  const truncated = [1,80,0,0], extended = [1,80,0,0,0,88,1,66,1,20,0,0,0];
  const refuses = (info) => { try { parseInfo(Uint8Array.from(info)); return false; } catch { return true; } };
  ok(parseInfo(Uint8Array.from(empty)).slots[0].nz === 0, "SMP_INFO genuine empty");
  ok(parseInfo(Uint8Array.from(occupied)).slots[0].nz === 1, "SMP_INFO genuine occupied");
  ok(parseInfo(Uint8Array.from(extended)).ext, "SMP_INFO extended genuine empty");
  ok(refuses(named), "SMP_INFO false empty with occupied name");
  ok(refuses(used), "SMP_INFO false empty with nonzero used size");
  ok(refuses(truncated), "SMP_INFO truncated empty descriptor");
  ok(refuses([2,80,0,0,0,0,76,79,79,80,0,1]), "SMP_INFO second slot false empty");
  const run = async (info) => {
    let writes = 0, reads = 0, closed = 0;
    const link = {
      drain() {}, send(frame) { return frame[4] === 15; },
      async read() { reads++; return Uint8Array.from([240,125,70,76,15,...info,247]); },
      close() { closed++; },
    };
    const id = { model: "SMK-37 Pro_900", text: "SMK-37 Pro_900" };
    const up = {
      async find(pred) { return pred(id) ? { id, link } : null; },
      async install() { writes++; return id.model; },
    };
    let error = null;
    try {
      await installKeepingSlots(up, new Uint8Array([0]), id.model, "smk", {
        sameKeyboard: () => true, confirmModel: () => true, beforeFirmwareWrite: () => true,
      });
    } catch (e) { error = e; }
    return { writes, reads, closed, error };
  };
  const good = await run(empty);
  ok(good.writes === 1 && good.closed === 1 && !good.error, "preflash genuine empty remains usable");
  for (const [name, info] of [["name",named],["used",used],["truncated",truncated]]) {
    const x = await run(info);
    ok(x.writes === 0 && x.reads === 1 && x.closed === 1 && x.error?.code === "slotlayout",
      `preflash corrupt ${name} refuses OTA and closes MIDI`);
  }
}

await packages();
await updater();
await sampleInfoEmptyGuard();
console.log(failed ? `WEB TESTS FAILED (${failed})` : "web tests passed");
process.exit(failed ? 1 : 0);
