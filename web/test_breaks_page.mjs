// SPDX-License-Identifier: GPL-3.0-only
// web/x0x_breaks.html run in node against a mocked device (no browser, no hardware): a stub DOM and a
// Web MIDI mock that follows firmware/src/app/editor.c (SMP_BEGIN .. SMP_READ, 11-16; NOR flash: erase
// sets 0xFF, a write only clears bits). Checks: Connect reads the slot layout from SMP_INFO; Back up
// sends only SMP_INFO / SMP_READ and gives the .x0xslots archive; Restore writes it back onto another
// device; Send uploads a new loop. Run from the repo root:
//   node web/test_breaks_page.mjs
import { readFileSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { crc32, parseHeader, readArchive, pack7 } from "./x0x_breaks.js";

let bad = 0;
const check = (c, what) => { console.log(`  ${c ? "ok  " : "FAIL"} ${what}`); if (!c) bad++; };

// ---- the page, with x0x_breaks.js inlined (tools/make_pages.py's way) ----
const js = execFileSync("python3", ["-c", `
import re, sys
sys.path.insert(0, "web")
from make_site import strip_module
page = open("web/x0x_breaks.html", encoding="utf-8").read()
lib = strip_module(open("web/x0x_breaks.js", encoding="utf-8").read())
print(re.search(r'<script type="module">(.*)</script>', page.replace("/*LIB*/", lib), re.S).group(1))
`]).toString();

// ---- mock device ----
const DATA_OFF = 512, HDR_LEN = 480;
function unpack7(a) {
  const out = [];
  for (let i = 0; i < a.length;) { const m = a[i++]; for (let j = 0; j < 7 && i < a.length; j++) out.push(a[i++] | ((m >> j) & 1) << 7); }
  return new Uint8Array(out);
}
class Device {
  constructor(count, size) {
    this.count = count; this.size = size; this.log = [];
    this.flash = Array.from({ length: count }, () => new Uint8Array(size).fill(0xFF));
    this.nz = new Array(count).fill(0);
  }
  scan(k) {
    const h = parseHeader(this.flash[k].subarray(0, HDR_LEN));
    this.nz[k] = h && h.dataLen <= this.size - DATA_OFF && crc32(this.flash[k].subarray(DATA_OFF, DATA_OFF + h.dataLen)) === h.crc ? h.nz : 0;
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
      const r = [this.count, (this.size / 1024) & 0x7F];
      for (let k = 0; k < this.count; k++) {
        const dl = new DataView(this.flash[k].buffer).getUint32(16, true);
        r.push(this.nz[k]);
        if (this.nz[k]) for (let i = 8; i < 16 && this.flash[k][i]; i++) r.push(this.flash[k][i] & 0x7F);
        r.push(0, this.nz[k] ? ((dl + 1023) >> 10) & 0x7F : 0);
      }
      return r;
    }
    const k = a[0];
    if (k >= this.count) return null;
    if (cmd === 16) {
      const off = a[1] | a[2] << 7 | a[3] << 14, n = Math.min(a[4] > 112 ? 224 : a[4] * 2, this.size - off);
      return [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F, ...pack7(this.flash[k].subarray(off, off + n))];
    }
    if (cmd === 11 || cmd === 14) {
      this.nz[k] = 0;
      for (let s = 0; s < (cmd === 14 ? this.size / 0x1000 : 1); s++) this.erase(k, s * 0x1000);
      return [k, 0];
    }
    if (cmd === 12) {
      const off = a[1] | a[2] << 7 | a[3] << 14, b = unpack7(a.slice(4));
      let rc = 0;
      if (off < DATA_OFF || off & 0xFF || !b.length || b.length > 256 || off + b.length > this.size) rc = 1;
      else if (this.nz[k]) rc = 4;
      else { if (!(off & 0xFFF)) this.erase(k, off); this.program(k, off, b); }
      return [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F, rc];
    }
    if (cmd === 13) {
      const h = unpack7(a.slice(1)), p = parseHeader(h);
      if (h.length !== HDR_LEN) return [k, 1];
      if (!p || p.dataLen > this.size - DATA_OFF) return [k, 2];
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
globalThis.getComputedStyle = () => ({ getPropertyValue: () => "#000" });
globalThis.confirm = () => true;
const blobs = [];
URL.createObjectURL = (b) => { blobs.push(b); return "blob:test"; };
URL.revokeObjectURL = () => {};
Object.defineProperty(globalThis, "navigator", { configurable: true, value: {
  requestMIDIAccess: async () => {
    const input = { name: "X0X FM-1", state: "connected", open: async () => {}, onmidimessage: null };
    const output = {
      name: "X0X FM-1", open: async () => {},
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
check(await idle(/Restored|differ|fit|damaged/), `Restore: "${$("status").textContent}"`);
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

console.log(bad ? "BREAKS PAGE FAILED" : "breaks page: connect, backup, restore ok against the mocked device");
process.exit(bad ? 1 : 0);
