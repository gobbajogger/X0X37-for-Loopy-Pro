// SPDX-License-Identifier: GPL-3.0-only
// web/x0x_breaks.html run in node against a mocked device (no browser, no hardware): a stub DOM and a
// Web MIDI mock that follows firmware/src/app/editor.c (SMP_BEGIN .. SMP_READ, 11-16; NOR flash: erase
// sets 0xFF, a write only clears bits). Checks: Connect reads the slot layout from SMP_INFO; Back up
// sends only SMP_INFO / SMP_READ and gives the .x0xslots archive; Restore writes it back onto another
// device; Send uploads a new loop at 44.1 kHz (22.05 kHz when asked). Run from the repo root:
//   node web/test_breaks_page.mjs
import { readFileSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { crc32, parseHeader, readArchive, pack78 } from "./x0x_breaks.js";

let bad = 0;
const check = (c, what) => { console.log(`  ${c ? "ok  " : "FAIL"} ${what}`); if (!c) bad++; };

// ---- the page, with x0x_breaks.js inlined (tools/make_pages.py's way) ----
const js = execFileSync("python3", ["-c", `
import re, sys
sys.path.insert(0, "web")
from make_site import strip_module
page = open("web/x0x_breaks.html", encoding="utf-8").read()
lib = "\\n".join(strip_module(open("web/" + f, encoding="utf-8").read()) for f in ("x0x_breaks.js", "device_session.js"))
print(re.search(r'<script type="module">(.*)</script>', page.replace("/*LIB*/", lib), re.S).group(1))
`]).toString();

// ---- mock device ----
const DATA_OFF = 512, HDR_LEN = 480;
function unpack7(a) {
  const out = [];
  for (let i = 0; i < a.length;) { const m = a[i++]; for (let j = 0; j < 7 && i < a.length; j++) out.push(a[i++] | ((m >> j) & 1) << 7); }
  return new Uint8Array(out);
}
// sizes: per slot (X0X's SMP_INFO extension, with flash offsets offs); without it count slots of one size
class Device {
  constructor(count, size, sizes = null, offs = null) {
    this.sizes = sizes || new Array(count).fill(size);
    this.count = this.sizes.length; this.size = Math.min(...this.sizes); this.offs = offs; this.log = [];
    this.flash = this.sizes.map((n) => new Uint8Array(n).fill(0xFF));
    this.nz = new Array(this.count).fill(0);
  }
  scan(k) {
    const h = parseHeader(this.flash[k].subarray(0, HDR_LEN));
    this.nz[k] = h && h.dataLen <= this.sizes[k] - DATA_OFF && crc32(this.flash[k].subarray(DATA_OFF, DATA_OFF + h.dataLen)) === h.crc ? h.nz : 0;
  }
  program(k, off, b) { for (let i = 0; i < b.length; i++) this.flash[k][off + i] &= b[i]; }
  erase(k, off) { this.flash[k].fill(0xFF, off, off + 0x1000); }
  load(k, img) { this.flash[k].fill(0xFF); this.program(k, 0, img); this.scan(k); }
  image(k) {
    if (!this.nz[k]) return null;
    const dl = new DataView(this.flash[k].buffer).getUint32(16, true);
    return this.flash[k].slice(0, DATA_OFF + dl);
  }
  handle(cmd, a) {
    this.log.push(cmd);
    if (cmd === 1) return [0];
    if (cmd === 15) {
      const r = [this.count, Math.min(127, this.size / 1024)], used = [];
      for (let k = 0; k < this.count; k++) {
        const dl = new DataView(this.flash[k].buffer).getUint32(16, true);
        used.push(this.nz[k] ? (dl + 1023) >> 10 : 0);
        r.push(this.nz[k]);
        if (this.nz[k]) for (let i = 8; i < 16 && this.flash[k][i]; i++) r.push(this.flash[k][i] & 0x7F);
        r.push(0, Math.min(127, used[k]));
      }
      if (this.offs) {
        r.push(0x58, 1);
        for (let k = 0; k < this.count; k++) for (const v of [this.offs[k] >> 12, this.sizes[k] >> 12, used[k]]) r.push(v & 0x7F, v >> 7);
      }
      return r;
    }
    const k = a[0];
    if (k >= this.count) return null;
    if (cmd === 16) {
      const off = a[1] | a[2] << 7 | a[3] << 14, n = Math.min(a[4] > 112 ? 224 : a[4] * 2, this.sizes[k] - off);
      return [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F, ...pack78(this.flash[k].subarray(off, off + n))];
    }
    if (cmd === 11 || cmd === 14) {
      this.nz[k] = 0;
      for (let s = 0; s < (cmd === 14 ? this.sizes[k] / 0x1000 : 1); s++) this.erase(k, s * 0x1000);
      return [k, 0];
    }
    if (cmd === 12) {
      const off = a[1] | a[2] << 7 | a[3] << 14, b = unpack7(a.slice(4));
      let rc = 0;
      if (off < DATA_OFF || off & 0xFF || !b.length || b.length > 256 || off + b.length > this.sizes[k]) rc = 1;
      else if (this.nz[k]) rc = 4;
      else { if (!(off & 0xFFF)) this.erase(k, off); this.program(k, off, b); }
      return [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F, rc];
    }
    if (cmd === 13) {
      const h = unpack7(a.slice(1)), p = parseHeader(h);
      if (h.length !== HDR_LEN) return [k, 1];
      if (!p || p.dataLen > this.sizes[k] - DATA_OFF) return [k, 2];
      if (crc32(this.flash[k].subarray(DATA_OFF, DATA_OFF + p.dataLen)) !== p.crc) return [k, 3];
      this.program(k, 0, h);
      this.scan(k);
      return [k, this.nz[k] ? 0 : 5];
    }
    return null;
  }
}

// ---- stub DOM and Web MIDI ----
function el(tag = "div") {
  const e = {
    tag, textContent: "", innerHTML: "", className: "", style: {}, disabled: false, hidden: false, value: 0, checked: false,
    multiple: false, title: "", children: [], width: 900, height: 80, href: "", download: "",
    classList: { add() {}, remove() {} },
    appendChild(c) { this.children.push(c); }, click() { if (this.onclickStub) this.onclickStub(); },
    querySelector() { return el(); }, getContext() { return { clearRect() {}, fillRect() {}, fillStyle: "" }; },
  };
  return e;
}
const ids = {};
let downloaded = null, device = null;
globalThis.document = {
  getElementById: (id) => (ids[id] ||= el()),
  createElement: (t) => { const e = el(t); if (t === "a") e.onclickStub = () => { downloaded = { name: e.download }; }; return e; },
  documentElement: {}, body: {},
};
ids.bars = el(); ids.bars.checked = true;
ids.half = el(); ids.trim = el();
globalThis.getComputedStyle = () => ({ getPropertyValue: () => "#000" });
globalThis.confirm = () => true;
const blobs = [];
URL.createObjectURL = (b) => { blobs.push(b); return "blob:test"; };
URL.revokeObjectURL = () => {};
Object.defineProperty(globalThis, "navigator", { configurable: true, value: {
  locks: {request: async (_name, _opts, fn) => fn({})},
  requestMIDIAccess: async () => {
    const input = { id: "i", name: "X0X FM-1", state: "connected", open: async () => {}, onmidimessage: null };
    const output = {
      id: "o", state: "connected", name: "X0X FM-1", open: async () => {},
      send(d) {
        const r = device.handle(d[4], d.slice(5, d.length - 1));
        if (r) setTimeout(() => input.onmidimessage({ data: new Uint8Array([0xF0, 0x7D, 0x46, 0x4C, d[4], ...r, 0xF7]) }), 0);
      },
    };
    return { inputs: new Map([["i", input]]), outputs: new Map([["o", output]]) };
  },
} });
const page = await import("data:text/javascript;base64," + Buffer.from(js).toString("base64"));
void page;
const $ = (id) => ids[id];
const idle = async (want, ms = 20000) => {
  const t0 = Date.now();
  while (Date.now() - t0 < ms) {
    await new Promise((r) => setTimeout(r, 5));
    if (!$("connect").disabled && (!want || want.test($("status").textContent))) return true;
  }
  return false;
};

// ---- 1. a 2 x 80 KB device with one slot of two loops; Connect; Back up ----
const img1 = (() => {
  const out = execFileSync("python3", ["-c", `
import sys
sys.path.insert(0, "tools")
import sampleio as sio
h, d = sio.user_slot("BR1", [([int(4000 * ((i * 7) % 13 - 6)) for i in range(30001)], 36, None, None),
                             ([int(3000 * ((i * 5) % 11 - 5)) for i in range(20000)], 37, None, None)], rate=44100)
sys.stdout.buffer.write(h.ljust(512, bytes([255])) + d)
`]);
  return new Uint8Array(out);
})();
device = new Device(2, 80 * 1024);
device.load(0, img1);
const before = device.flash.map((f) => f.slice());
await $("connect").onclick();
check(await idle(), "Connect: the page reads the slots");
check(/2 slots/.test($("room").textContent), `room text from SMP_INFO: "${$("room").textContent}"`);
device.log = [];
await $("backup").onclick();
check(device.log.every((c) => c === 15 || c === 16) && device.log.includes(16), "Back up sends only SMP_INFO and SMP_READ");
check(device.flash.every((f, k) => f.every((v, i) => v === before[k][i])), "Back up leaves the flash unchanged");
const arch = new Uint8Array(await blobs[blobs.length - 1].arrayBuffer());
const { meta, images } = readArchive(arch);
check(meta.count === 2 && meta.size === 81920 && images[0].length === img1.length && images[0].every((v, i) => v === img1[i]) &&
  images[1] === null && /\.x0xslots$/.test(downloaded.name), `Back up: archive ${downloaded.name}, slot 1 exact, slot 2 empty`);

// ---- 2. Restore onto a device that holds other content in both slots ----
const other = device.image(0).slice();
device = new Device(2, 80 * 1024);
device.load(0, other);
device.load(1, other);
await $("connect").onclick();
await idle();
$("archive").onchange({ target: { files: [{ name: "b.x0xslots", arrayBuffer: async () => arch.buffer }], value: "" } });
check(await idle(/Complete restore|PARTIAL|differ|fit|damaged/), `Restore: "${$("status").textContent}"`);
const im0 = device.image(0);
check(im0 && im0.length === img1.length && im0.every((v, i) => v === img1[i]) && device.image(1) === null,
  "Restore: slot 1 equals the backup, slot 2 erased");

// ---- 3. a 3 x 120 KB device: the page follows SMP_INFO ----
device = new Device(3, 120 * 1024);
device.load(2, img1);
await $("connect").onclick();
await idle();
check(/3 slots/.test($("room").textContent) && /359 KB/.test($("room").textContent), `3 x 120 KB layout: "${$("room").textContent}"`);
await $("backup").onclick();
const m3 = readArchive(new Uint8Array(await blobs[blobs.length - 1].arrayBuffer()));
check(m3.meta.count === 3 && m3.meta.size === 120 * 1024 && m3.images[2] !== null && m3.images[0] === null,
  "3 x 120 KB layout: backup holds 3 slots, slot 3 used");

// ---- 4. Send: a new loop goes in at 44.1 kHz; with "22 kHz" ticked at 22.05 kHz; a loop read back keeps its rate ----
function wavFile(name, seconds, sr = 48000) {
  const n = Math.round(sr * seconds), b = Buffer.alloc(44 + 2 * n);
  b.write("RIFF", 0); b.writeUInt32LE(36 + 2 * n, 4); b.write("WAVEfmt ", 8); b.writeUInt32LE(16, 16); b.writeUInt16LE(1, 20);
  b.writeUInt16LE(1, 22); b.writeUInt32LE(sr, 24); b.writeUInt32LE(2 * sr, 28); b.writeUInt16LE(2, 32); b.writeUInt16LE(16, 34);
  b.write("data", 36); b.writeUInt32LE(2 * n, 40);
  for (let i = 0; i < n; i++) b.writeInt16LE(Math.round(12000 * Math.sin(i * 0.05) * (i % 6000 < 3000 ? 1 : 0.2)), 44 + 2 * i);
  return { name, arrayBuffer: async () => b.buffer.slice(b.byteOffset, b.byteOffset + b.length) };
}
device = new Device(2, 80 * 1024);
device.load(0, img1);                                   // two 44.1 kHz loops
await $("connect").onclick();
await idle();
$("bars").checked = false;
$("files").onchange({ target: { files: [wavFile("a.wav", 0.6)], value: "" } });
await new Promise((r) => setTimeout(r, 300));
await $("send").onclick();
await idle(/Done|failed|didn't/);
let h0 = parseHeader(device.flash[0].subarray(0, 480));
check(h0 && h0.nz === 3 && h0.zones[2].rate === 44100 && h0.zones[2].n === Math.floor(0.6 * 48000 * 44100 / 48000),
  `Send: the new loop is zone 3 at 44.1 kHz (${h0 && h0.zones[2].n} samples)`);
$("half").checked = true;
$("files").onchange({ target: { files: [wavFile("b.wav", 0.6)], value: "" } });
await new Promise((r) => setTimeout(r, 300));
await $("send").onclick();
await idle(/Done|failed|didn't/);
h0 = parseHeader(device.flash[0].subarray(0, 480));
check(h0 && h0.nz === 4 && h0.zones.map((z) => z.rate).join() === "44100,44100,44100,22050",
  `Send with "22 kHz": rates ${h0 && h0.zones.map((z) => z.rate).join()} (read-back loops keep theirs)`);
$("half").checked = false;

// ---- 5. the SMK-37 layout (80 / 120 / 44 KB, SMP_INFO extension): sizes per slot; a loop too long for slot 1 ----
device = new Device(0, 0, [0x14000, 0x1E000, 0xB000], [0xC2000, 0xD6000, 0xB0000]);
device.load(1, img1);
await $("connect").onclick();
await idle();
check(/3 slots: 80 KB, 120 KB, 44 KB/.test($("room").textContent) && /243 KB/.test($("room").textContent),
  `SMK-37 layout: "${$("room").textContent}"`);
await $("backup").onclick();
const m5 = readArchive(new Uint8Array(await blobs[blobs.length - 1].arrayBuffer()));
check(m5.meta.slots.map((e) => e.size).join() === "81920,122880,45056" && m5.meta.slots[2].flash === 0xB0000 && m5.images[1] !== null,
  "SMK-37 layout: the backup records each slot's size and flash offset");
$("bars").checked = false;
$("files").onchange({ target: { files: [wavFile("long.wav", 3.8)], value: "" } });   // about 82 KB: more than slot 1 holds
await new Promise((r) => setTimeout(r, 500));
await $("send").onclick();
await idle(/Done|failed|didn't|Too much/);
const h5 = [0, 1, 2].map((k) => parseHeader(device.flash[k].subarray(0, 480)));
check(device.nz[0] && h5[0].nz === 2 && device.nz[1] && h5[1].nz === 1 && h5[1].dataLen > 80 * 1024 - 512,
  `SMK-37 layout: the two loops read back go into slot 1, the long new one into the 120 KB slot 2 (${h5[1] && h5[1].dataLen} B; "${$("status").textContent}")`);

console.log(bad ? "BREAKS PAGE FAILED" : "breaks page: connect, backup, restore, send ok against the mocked device");
process.exit(bad ? 1 : 0);
