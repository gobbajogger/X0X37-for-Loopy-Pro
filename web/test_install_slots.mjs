// SPDX-License-Identifier: GPL-3.0-only
// The installer keeps the sample slots over an install that changes the slot map (no browser, no hardware,
// no device writes): x0x_breaks.js installKeepingSlots with fm1ota.js's Updater, and the installer page
// (web/x0x_installer.html with the libraries inlined as tools/make_pages.py does) on a stub DOM, against a
// simulated SMK-37: the update protocol of test_web.mjs's FakeFM1, and the slot commands of
// firmware/src/app/editor.c over one NOR flash model, so the slots stay over an install as on the device.
// Each firmware has its slot map: this X0X 80/120/44 KiB with the SMP_INFO extension, an older X0X
// 2 x 80 KiB without it, stock firmware none. Run from the repo root: node web/test_install_slots.mjs
import "./test_install_nightly_download_guard.mjs";
import "./test_install_nightly_product_identity.mjs";
import "./test_install_nightly_reconnect.mjs";
import "./test_install_stock_model_preflight.mjs";
import "./test_install_nightly_scan_order.mjs";
import "./test_install_nightly_ota_model.mjs";
import "./test_install_nightly_local_selection.mjs";
import "./test_install_nightly_controller_selection.mjs";
import "./test_install_nightly_slot_prewrite.mjs";
import { execFileSync } from "node:child_process";
import { createHash } from "node:crypto";
import { appendFileSync, mkdtempSync, readFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { Updater, pack7, unpack7, sameKeyboard } from "./fm1ota.js";
import { crc32, parseHeader, readArchive, pack78, unpack78, installKeepingSlots, packageSlotMap } from "./x0x_breaks.js";

// X0X_TEST_TIME_SCALE=N (opt-in, default 1): waits of 1000 ms or more (the updater's fixed waits for a reboot
// and its read timeouts) take 1/N of the time. The mock keyboard replies at once, so it does not use these waits.
const TIME_SCALE = Number(process.env.X0X_TEST_TIME_SCALE || 1);
if (TIME_SCALE > 1) {
  const realSetTimeout = globalThis.setTimeout;
  globalThis.setTimeout = (f, ms = 0, ...a) => realSetTimeout(f, ms >= 1000 ? ms / TIME_SCALE : ms, ...a);
}
let bad = 0;
const ok = (c, what) => { console.log(`${what.padEnd(70)} ${c ? "ok" : "FAIL"}`); if (!c) bad++; };
const eq = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
const HS = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];
const DATA_OFF = 512, HDR_LEN = 480;

const NEW = { map: [[0xC2000, 0x14000], [0xD6000, 0x1E000], [0xB0000, 0xB000]], ext: true };
const OLD = { map: [[0xC2000, 0x14000], [0xD6000, 0x14000]], ext: false };
const MOVED = { map: [[0xC2000, 0x14000], [0xD7000, 0x1D000], [0xB0000, 0xB000]], ext: true };
const NEW_PKG = "SMK-37 Pro_9001100", OLD_PKG = "SMK-37 Pro_9001000", NOW = "SMK-37 Pro_9001050";

// ---- the slot images (tools/sampleio.py, as an upload leaves them) ----
const dir = mkdtempSync(join(tmpdir(), "x0x-install-slots-"));
execFileSync("python3", ["-c", `
import math, sys
sys.path.insert(0, "tools")
import sampleio as sio
for k, (n, seed, size) in enumerate([(120000, 3, 0x14000), (200000, 5, 0x1E000), (60000, 7, 0xB000)]):
    s = [int(9000 * math.sin(0.01 * seed * i)) + ((i * seed) % 97) - 48 for i in range(n)]
    h, d = sio.user_slot("BR%d" % (k + 1), [(s, 36, None, None)], rate=44100, slot_size=size)
    open(sys.argv[1] + "/s%d" % k, "wb").write(h.ljust(512, bytes([255])) + d)
`, dir]);
const IMG = [0, 1, 2].map((k) => new Uint8Array(readFileSync(join(dir, `s${k}`))));

// ---- a synthetic package: the identity markers (fm1pkg.js productOf), optional loader mark and slot map ----
function packageOf(product, { marker = true, map = null } = {}) {
  const raw = Uint8Array.from({ length: 0x2000 + 20 }, (_, i) => (i * 7) & 0xFF);
  for (let i = 0; i < 20; i++) raw[i * 0x30 + 0x2F] = i < product.length ? (product.charCodeAt(i) + i + 1) & 0xFF : 0x7D;
  raw.fill(0, 0x1800, 0x1810);
  if (marker) raw.set([..."FELUCCA-LOADER-1"].map((c) => c.charCodeAt(0)), 0x1800);
  if (map) {
    const b = new Uint8Array(20 + 8 * map.length + 4), v = new DataView(b.buffer);
    b.set([..."X0X-SLOTMAP-1\0\0\0"].map((c) => c.charCodeAt(0)));
    v.setUint32(16, map.length, true);
    map.forEach(([o, n], k) => { v.setUint32(20 + 8 * k, o, true); v.setUint32(24 + 8 * k, n, true); });
    v.setUint32(b.length - 4, crc32(b.subarray(0, b.length - 4)), true);
    raw.set(b, 0x1A00);
  }
  const logical = [];
  for (let i = 0; i < 20; i++) logical.push(...raw.subarray(i * 0x30, i * 0x30 + 0x2F));
  return { raw, image: Uint8Array.from([...logical, ...raw.subarray(20 * 0x30)]) };
}

// ---- the simulated keyboard ----
class Flash {
  constructor() { this.mem = new Uint8Array(0x100000).fill(0xFF); }
  program(o, b) { for (let i = 0; i < b.length; i++) this.mem[o + i] &= b[i]; }
  erase(o) { this.mem.fill(0xFF, o, o + 0x1000); }
}
function slotImages(flash, fw) {
  return fw.map.map(([o, n]) => {
    const h = parseHeader(flash.mem.subarray(o, o + HDR_LEN));
    if (!h || h.dataLen > n - DATA_OFF || crc32(flash.mem.subarray(o + DATA_OFF, o + DATA_OFF + h.dataLen)) !== h.crc) return null;
    return flash.mem.slice(o, o + DATA_OFF + h.dataLen);
  });
}
function load(flash, fw, k, img) {
  const [o, n] = fw.map[k];
  flash.mem.fill(0xFF, o, o + n);
  flash.program(o, img);
}

class FakeX0X {
  constructor(image, identity, afterWrite, firmware, flash, { failWriteAt = null, stockErase = null } = {}) {
    Object.assign(this, { image, afterWrite, firmware, flash, failWriteAt, stockErase });
    this.access = { inputs: new Map(), outputs: new Map() };
    this.slotLog = []; this.upgrades = 0; this.bad = 0;
    this.boot(identity, "SMK-37 Pro");
  }
  boot(identity, name) {
    this.identity = identity; this.waiting = null; this.queue = [];
    this.fw = this.firmware[identity] || null;
    if (!this.fw && !identity.startsWith("ota-") && this.stockErase) for (let o = this.stockErase[0]; o < this.stockErase[1]; o += 0x1000) this.flash.erase(o);
    this.fail = identity === this.afterWrite ? this.failWriteAt : null;
    for (const m of [this.access.inputs, this.access.outputs]) { for (const p of m.values()) p.state = "disconnected"; m.clear(); }
    const id = Math.random().toString(36).slice(2);
    this.input = { id: "i" + id, name, state: "connected", onmidimessage: null, open: async () => {} };
    this.output = { id: "o" + id, name, state: "connected", open: async () => {}, send: (d) => {
      if (this.output.state !== "connected") throw new Error("InvalidStateError");
      setImmediate(() => this.rx(Array.from(d)));          // async as a device, without the 1 ms of setTimeout
    } };
    this.access.inputs.set(this.input.id, this.input);
    this.access.outputs.set(this.output.id, this.output);
  }
  tx(bytes) { const i = this.input; setImmediate(() => { if (i.state === "connected" && i.onmidimessage) i.onmidimessage({ data: Uint8Array.from(bytes) }); }); }
  rx(d) {
    if (d[1] === 0x7D && d[2] === 0x46 && d[3] === 0x4C) {
      if (!this.fw) return;                                  // stock firmware or the loader: no answer
      this.slotLog.push([this.identity, d[4]]);
      const r = this.slot(d[4], d.slice(5, -1));
      if (r) this.tx([0xF0, 0x7D, 0x46, 0x4C, d[4], ...r, 0xF7]);
      return;
    }
    if (eq(d, HS)) {
      const t = [...new TextEncoder().encode(this.identity)];
      const payload = [...t, ...new Array(27 - t.length).fill(0)];
      const body = [0, 0x59, 0x11, 27, 0, 0, ...payload, ~payload.reduce((sum, b) => sum + b, 0) & 255];
      this.tx([0xF0, ...pack7(body), 0xF7]);
    } else if (eq(d, UPGRADE)) {
      this.upgrades++;
      this.queue = this.identity.startsWith("ota-") ? [...Array.from({ length: 6 }, (_, k) => [k * 512, 512]), [0xF0000000, 8]]
        : [[0, 64], [0x40, 160], [0x1000, 512], [0xE0000000, 8]];
      this.next();
    } else if (this.waiting) {
      const u = unpack7(d.slice(1, -1)), [addr, len] = this.waiting;
      const want = addr >= 0xE0000000 ? [...new TextEncoder().encode("success"), 0] : Array.from(this.image.subarray(addr, addr + len));
      if (!eq(u.slice(14, 14 + want.length), want)) this.bad++;
      this.waiting = null;
      if (addr === 0xE0000000) setTimeout(() => this.boot("ota-SMK-37 Pro_900", "Felucca Update"), 50);
      else if (addr === 0xF0000000) setTimeout(() => this.boot(this.afterWrite, "SMK-37 Pro"), 50);
      else this.next();
    }
  }
  next() {
    const r = this.queue.shift();
    if (!r) return;
    this.waiting = r;
    const [addr, len] = r;
    const u = [0, 0x59, 0x30, 8, 0, 0, 0, addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF, len & 0xFF, len >> 8, 0];
    let s = 0;
    for (let i = 6; i < 14; i++) s += u[i];
    u.push(~s & 0xFF);
    this.tx([0xF0, ...pack7(u), 0xF7]);
  }
  nz(k) {
    const [o, n] = this.fw.map[k], h = parseHeader(this.flash.mem.subarray(o, o + HDR_LEN));
    return h && h.dataLen <= n - DATA_OFF && crc32(this.flash.mem.subarray(o + DATA_OFF, o + DATA_OFF + h.dataLen)) === h.crc ? h : null;
  }
  slot(cmd, a) {                                             // editor.c's slot commands, as tests/slot_mock.py
    const map = this.fw.map;
    if (cmd === 15) {
      const r = [map.length, Math.min(127, Math.min(...map.map(([, n]) => n)) >> 10)], used = [];
      map.forEach((_, k) => {
        const h = this.nz(k);
        used.push(h ? (h.dataLen + 1023) >> 10 : 0);
        r.push(h ? h.nz : 0, ...(h ? [...h.name].map((c) => c.charCodeAt(0) & 0x7F) : []), 0, Math.min(127, used[k]));
      });
      if (this.fw.ext) {
        r.push(0x58, 1);
        map.forEach(([o, n], k) => { for (const v of [o >> 12, n >> 12, used[k]]) r.push(v & 0x7F, (v >> 7) & 0x7F); });
      }
      return r;
    }
    const k = a[0];
    if (k >= map.length) return null;
    const [base, size] = map[k];
    if (cmd === 16) {
      const off = a[1] | a[2] << 7 | a[3] << 14, n = Math.min(a[4] > 112 ? 224 : a[4] * 2, size - off);
      return [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F, ...pack78(this.flash.mem.subarray(base + off, base + off + n))];
    }
    if (cmd === 11 || cmd === 14) {
      for (let s = 0; s < (cmd === 14 ? size >> 12 : 1); s++) this.flash.erase(base + s * 0x1000);
      return [k, 0];
    }
    if (cmd === 12) {
      const off = a[1] | a[2] << 7 | a[3] << 14, b = unpack78(a.slice(4));
      let rc = 0;
      if (off < DATA_OFF || off & 0xFF || !b.length || b.length > 256 || off + b.length > size) rc = 1;
      else if (this.nz(k)) rc = 4;
      else if (this.fail && this.fail[0] === k && this.fail[1] === off) rc = 3;
      else { if (!(off & 0xFFF)) this.flash.erase(base + off); this.flash.program(base + off, b); }
      return [k, off & 0x7F, (off >> 7) & 0x7F, (off >> 14) & 0x7F, rc];
    }
    if (cmd === 13) {
      const h = unpack78(a.slice(1)), p = parseHeader(h);
      if (h.length !== HDR_LEN) return [k, 1];
      if (!p || p.dataLen > size - DATA_OFF) return [k, 2];
      if (crc32(this.flash.mem.subarray(base + DATA_OFF, base + DATA_OFF + p.dataLen)) !== p.crc) return [k, 3];
      this.flash.program(base, h);
      return [k, this.nz(k) ? 0 : 5];
    }
    return null;
  }
}

const same = (a, b) => (a === null || b === null ? a === b : eq(a, b));
// X0X_SLOT_DUMP=<file>: write one line per install (the CRC-32 of the whole flash model and of each slot image in
// each saved file), so that two runs of this test can be compared. Not set: no file is written.
let dumps = 0;
function dump(kind, flash, files) {
  if (!process.env.X0X_SLOT_DUMP) return;
  const h = (b) => (crc32(b) >>> 0).toString(16).padStart(8, "0");
  const fl = files.map((b) => { const a = readArchive(b); return a ? a.images.map((im) => (im ? h(im) : "-")).join("/") : "no-archive"; });
  appendFileSync(process.env.X0X_SLOT_DUMP, [String(++dumps).padStart(2, "0"), kind, "flash", h(flash.mem), "files", ...fl].join(" ") + "\n");
}
async function flow(pkg, product, from, to, firmware, fill, opts = {}) {
  const flash = new Flash();
  fill.forEach(([k, img]) => load(flash, firmware[from], k, img));
  const dev = new FakeX0X(pkg.image, from, to, firmware, flash, opts);
  const saved = [], said = [];
  let res = null, err = null;
  try {
    res = await installKeepingSlots(new Updater(dev.access), pkg.image, product, "smk",
      { sameKeyboard, save: (b, name) => saved.push({ b, name }), say: (k, a) => said.push([k, a]) });
  } catch (e) { err = e; }
  dump("flow", flash, saved.map((x) => x.b));
  return { flash, dev, saved, said, res, err };
}

// ---- 1. the package slot map ----
ok(packageSlotMap(packageOf(NEW_PKG, { map: NEW.map }).image).map((m) => [m.off, m.size]).join() === NEW.map.join()
  && packageSlotMap(packageOf(OLD_PKG).image).map((m) => [m.off, m.size]).join() === OLD.map.join()
  && packageSlotMap(packageOf("SMK-37 Pro_016", { marker: false }).image).length === 0,
  "packageSlotMap: the record, else the old 2 x 80 KiB (loader mark), else none");

// ---- 2. an older X0X -> this X0X: every slot stays, nothing saved, SMP_INFO only ----
{
  const fw = { [OLD_PKG]: OLD, [NEW_PKG]: NEW };
  const r = await flow(packageOf(NEW_PKG, { map: NEW.map }), NEW_PKG, OLD_PKG, NEW_PKG, fw, [[0, IMG[0]], [1, IMG[2]]]);
  ok(!r.err && !r.res.move && !r.saved.length && r.dev.slotLog.every(([, c]) => c === 15)
    && same(slotImages(r.flash, NEW)[0], IMG[0]) && same(slotImages(r.flash, NEW)[1], IMG[2]),
    "upgrade: slots kept in place, no backup, SMP_INFO only");
}

// ---- 3. this X0X -> an older X0X: save, install, slot 3 back into the free slot 2, slot 2 (98 KiB) reported ----
{
  const fw = { [NOW]: NEW, [OLD_PKG]: OLD };
  const r = await flow(packageOf(OLD_PKG), OLD_PKG, NOW, OLD_PKG, fw, [[0, IMG[0]], [1, IMG[1]], [2, IMG[2]]]);
  const arch = r.saved[0] && readArchive(r.saved[0].b);
  ok(!r.err && arch && arch.images.every((im, k) => same(im, IMG[k])) && /^x0x-slots-before-install-\d{8}-\d{6}\.x0xslots$/.test(r.saved[0].name),
    `downgrade: every slot saved (${r.saved[0] && r.saved[0].name})`);
  const before = r.dev.slotLog.filter(([id]) => id === NOW).map(([, c]) => c);
  ok(before.every((c) => c === 15 || c === 16) && before.includes(16), "downgrade: the backup sends SMP_INFO / SMP_READ only");
  const now = slotImages(r.flash, OLD);
  ok(r.res && same(now[0], IMG[0]) && now[1] === null && r.res.move.todo.length === 0
    && r.res.move.skipped.length === 2 && r.res.move.skipped.some(s => /LOOP menu order or indices/.test(s))
    && r.dev.slotLog.filter(([id]) => id === OLD_PKG).every(([, cmd]) => cmd === 15 || cmd === 16),
    "downgrade: skipped middle loop blocks all restore writes and preserves archive for reconciliation");
  ok(r.said.some(([k, a]) => k === "slotsave" && a === "2, 3"), "downgrade: the page is told which slots are at risk");
}

// ---- 4. a later map moves slot 2: saved, installed, written back at its new offset ----
{
  const fw = { [NOW]: NEW, [NEW_PKG]: MOVED };
  const r = await flow(packageOf(NEW_PKG, { map: MOVED.map }), NEW_PKG, NOW, NEW_PKG, fw, [[0, IMG[0]], [1, IMG[1]]]);
  const now = slotImages(r.flash, MOVED);
  ok(!r.err && r.saved.length === 1 && same(now[0], IMG[0]) && same(now[1], IMG[1]) && r.res.move.skipped.length === 0,
    "moved slot: saved, installed, slot 2 written back at 0xD7000 and verified");
}

// ---- 5. failure mid-restore: code 'slots', the archive was saved before the install and names the file ----
{
  const fw = { [NOW]: NEW, [NEW_PKG]: MOVED };
  const r = await flow(packageOf(NEW_PKG, { map: MOVED.map }), NEW_PKG, NOW, NEW_PKG, fw, [[0, IMG[0]], [1, IMG[1]]], { failWriteAt: [1, 0x4200] });
  ok(r.err && r.err.code === "slots" && r.err.detail === r.saved[0].name && /write at 0x4200 failed/.test(r.err.message),
    `failure mid-restore: error 'slots' (${r.err && r.err.message})`);
  const arch = readArchive(r.saved[0].b);
  ok(same(arch.images[0], IMG[0]) && same(arch.images[1], IMG[1]) && r.dev.upgrades === 2,
    "failure mid-restore: the saved archive holds the slots, the install itself completed");
}

// ---- 6. the backup fails: nothing is installed ----
{
  const fw = { [NOW]: NEW, [OLD_PKG]: OLD };
  const flash = new Flash();
  load(flash, NEW, 1, IMG[1]);
  const dev = new FakeX0X(packageOf(OLD_PKG).image, NOW, OLD_PKG, fw, flash);
  const slot = dev.slot.bind(dev);
  dev.slot = (cmd, a) => (cmd === 16 ? null : slot(cmd, a));             // SMP_READ never answers
  const e = await installKeepingSlots(new Updater(dev.access), packageOf(OLD_PKG).image, OLD_PKG, "smk", { sameKeyboard }).then(() => null, (x) => x);
  ok(e && e.code === "slotbackup" && dev.upgrades === 0, "backup fails: error 'slotbackup', nothing sent to install");
}

// ---- 7. stock firmware: every slot saved, nothing written back (its map is empty) ----
{
  const fw = { [NOW]: NEW };
  const r = await flow(packageOf("SMK-37 Pro_016", { marker: false }), "SMK-37 Pro_016", NOW, "SMK-37 Pro_016", fw,
    [[0, IMG[0]], [2, IMG[2]]], { stockErase: [0x9D000, 0xC0000] });
  const arch = r.saved[0] && readArchive(r.saved[0].b);
  ok(!r.err && arch && same(arch.images[0], IMG[0]) && arch.images[1] === null && same(arch.images[2], IMG[2])
    && r.res.move.newMap.length === 0 && r.dev.slotLog.every(([id]) => id === NOW),
    "stock package: all slots saved, nothing written back");
}

// ---- 8. the installer page itself (stub DOM): the download, the status, the instructions ----
const strip = (src) => src.replace(/^export\s+/gm, "").replace(/^import .*?;\n/gms, "");
async function page(pkg, product, firmware, from, fill, opts = {}) {
  // Match tools/make_pages.py LIB order (boot preview + factory loops).
  const lib = ["fm1pkg.js", "fm1ota.js", "x0x_breaks.js", "factory_break_pack.js", "boot_screen.js", "device_session.js"].map((f) => strip(readFileSync(new URL(f, import.meta.url), "utf8"))).join("\n");
  const html = readFileSync(new URL("x0x_installer.html", import.meta.url), "utf8");
  const model = /^SMK-37 Elite/i.test(product) ? "elite" : /^MKE-P37/i.test(product) ? "mke"
    : /^FM-1/i.test(product) ? "fm1" : "pro";
  // Only this simulated-device test injects a reviewed-source permission.
  // The real NIGHTLY source keeps its immutable publisher allowlist EMPTY.
  // This tests the full backup/write/restore journey *after* the picker gate.
  const pkgSha = createHash("sha256").update(pkg.raw).digest("hex");
  const ctlSha = "b".repeat(64), rootSha = "1".repeat(40), treeSha = "2".repeat(40);
  const pkgUrl = "https://nightly.example.test/sha256/test.fwsc";
  const entry = { product, pkg: pkgUrl, size: pkg.raw.length, sha256: pkgSha, controller_sha256: ctlSha };
  const catalog = JSON.stringify({
    repo: "https://github.com/gobbajogger/X0X37-for-Loopy-Pro",
    latest: "test",
    versions: [{ version: "test", status: "dev-checked", source_commit: rootSha,
      firmware_tree_sha: treeSha, qa_pass: true, source_licenses_approved: true,
      models: { [model]: entry } }],
  });
  const approval = {
    prehardware_qa_pass: true, license_and_source_ok: true, backup_integrity_pass: true,
    model_and_controller_pass: true, immutable_http_pass: true, model, product,
    source_root_sha: rootSha, firmware_tree_sha: treeSha,
    package_url: pkgUrl, package_size: entry.size, package_sha256: pkgSha, controller_sha256: ctlSha, host_sha256: pkgSha,
  };
  const source = html.replace("const NIGHTLY_PUBLISHER_APPROVALS = Object.freeze({});",
    `const NIGHTLY_PUBLISHER_APPROVALS = Object.freeze(${JSON.stringify({ [`test/${model}`]: approval })});`);
  if (source === html) throw new Error("test source publisher guard not found");
  const js = /<script type="module">([\s\S]*)<\/script>/.exec(source.replace("/*LIB*/", lib).replace("/*CATALOG*/", catalog))[1];
  const flash = new Flash();
  fill.forEach(([k, img]) => load(flash, firmware[from], k, img));
  const dev = new FakeX0X(pkg.image, from, product, firmware, flash, opts);
  const ids = {}, downloads = [], radios = [];
  const el = (tag = "div") => {
    const o = {
      tag, textContent: "", value: "", disabled: false, hidden: false, checked: false, className: "",
      dataset: {}, handlers: {}, children: [], style: {},
      // Boot preview canvas; installer draws at import time.
      getContext() { return { clearRect() {}, fillRect() {}, drawImage() {}, fillText() {},
        measureText: () => ({ width: 0 }), save() {}, restore() {}, beginPath() {}, closePath() {},
        moveTo() {}, lineTo() {}, stroke() {}, fill() {}, translate() {}, scale() {},
        putImageData() {}, getImageData: () => ({ data: new Uint8ClampedArray(4), width: 1, height: 1 }),
        createImageData: (w, h) => ({ data: new Uint8ClampedArray(w * h * 4), width: w, height: h }),
        canvas: { width: 128, height: 64 } }; },
      classList: {toggle() {},add(){},remove(){}},
      addEventListener(t, f) { this.handlers[t] = f; },
      append(...nodes) { for (const n of nodes) this.children.push(n); },
      appendChild(n) { this.children.push(n); return n; },
      remove() {},
      removeAttribute() {},
      setAttribute(name, value) { this[name] = value; if (name === "id") this.id = value; },
      focus() {},
      click() { if (this.download) downloads.push({ name: this.download, href: this.href, blob: this.blob }); },
      set innerHTML(v) { this._html = v; this.children = []; this.value = ""; },
      get innerHTML() { return this._html || ""; },
      // Installer backup readback inserts a panel after #status; auto-confirm it.
      after(node) {
        this._after = this._after || [];
        this._after.push(node);
        if (node && node.id === "backupverify") {
          queueMicrotask(async () => {
            const link = node.children.find((c) => c && c.download);
            const chooser = node.children.find((c) => c && c.type === "file");
            if (!link || !chooser || !chooser.handlers.change) return;
            const blob = blobs.get(link.href);
            if (!blob) return;
            const bytes = new Uint8Array(await blob.arrayBuffer());
            link.click();
            Object.defineProperty(chooser, "files", {
              configurable: true,
              get: () => [{
                name: link.download || "backup.x0xslots",
                size: bytes.length,
                async arrayBuffer() { return bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength); },
              }],
            });
            await chooser.handlers.change();
          });
        }
      },
    };
    if (tag === "input") radios.push(o);
    return o;
  };
  globalThis.document = {
    getElementById: (id) => (ids[id] ||= el(id === "version" ? "select" : id === "bootprev" ? "canvas" : id === "models" ? "div" : "div")),
    querySelectorAll: () => [],
    querySelector: (sel) => {
      const m = /input\[value="([^"]+)"\]/.exec(sel);
      if (m) return radios.find((r) => r.value === m[1] && r.checked) || radios.find((r) => r.value === m[1]) || null;
      return null;
    },
    documentElement: {},
    body: { appendChild() {} },
    createElement: (tag) => el(tag),
    createTextNode: (t) => ({ textContent: t }),
  };
  // createElement("label") then append(input, text) — make label.append wire the radio value
  const realCreate = globalThis.document.createElement;
  globalThis.document.createElement = (tag) => {
    const node = realCreate(tag);
    if (tag === "label") {
      node.append = (...nodes) => {
        for (const n of nodes) {
          node.children.push(n);
          if (n && n.tag === "input") radios.push(n);
        }
      };
    }
    return node;
  };
  globalThis.window = { addEventListener() {}, confirm: () => true };
  if (typeof globalThis.ImageData !== "function") {
    globalThis.ImageData = class ImageData {
      constructor(dataOrW, wOrH, h) {
        if (typeof dataOrW === "number") {
          this.width = dataOrW; this.height = wOrH;
          this.data = new Uint8ClampedArray(this.width * this.height * 4);
        } else {
          this.data = dataOrW; this.width = wOrH; this.height = h;
        }
      }
    };
  }

  globalThis.fetch = async (url) => {
    if (String(url).includes("api.github.com")) return { ok: false, status: 404, json: async () => [] };
    // Mirror modern Fetch: approved remote firmware requires bounded streaming.
    return { ok: true, body: new Blob([pkg.raw]).stream() };
  };
  const blobs = new Map();
  URL.createObjectURL = (b) => { const u = `blob:${blobs.size}`; blobs.set(u, b); return u; };
  URL.revokeObjectURL = () => {};
  Object.defineProperty(globalThis, "navigator", { configurable: true, value: { locks: {request: async (_name, _opts, fn) => fn({})}, requestMIDIAccess: async () => dev.access } });
  await import("data:text/javascript;base64," + Buffer.from(js + `\n//${Math.random()}`).toString("base64"));
  for (let i = 0; i < 200 && ids.go?.disabled !== false; i++) await new Promise((r) => setTimeout(r, 10));
  if (!ids.go || ids.go.disabled !== false) throw new Error("installer page did not enable Install");
  if (!ids.go.handlers.click) throw new Error("installer page has no Install click handler");
  // Status is only created when say()/applyLang touch it; seed before click.
  const before = (ids.status ||= el()).textContent;
  const clicked = ids.go.handlers.click();
  for (let i = 0; i < 3000; i++) {
    if (ids.status.textContent && ids.status.textContent !== before && ids.go.disabled === false) break;
    if (/written back|Restore|Done|slots|error|Error/i.test(ids.status.textContent || "") && ids.go.disabled === false) break;
    await new Promise((r) => setTimeout(r, 10));
  }
  await Promise.resolve(clicked).catch(() => {});
  const files = [];
  for (const d of downloads) {
    const key = d.href || [...blobs.keys()].pop();
    files.push({ name: d.name, b: new Uint8Array(await blobs.get(key).arrayBuffer()) });
  }
  dump("page", flash, files.map((x) => x.b));
  return { status: ids.status.textContent, files, flash };
}
{
  const fw = { [NOW]: NEW, [NEW_PKG]: MOVED };
  const r = await page(packageOf(NEW_PKG, { map: MOVED.map }), NEW_PKG, fw, NOW, [[0, IMG[0]], [1, IMG[1]]]);
  ok(/slots were written back and checked/.test(r.status) && r.files.length === 1 && same(slotImages(r.flash, MOVED)[1], IMG[1]),
    `installer page: download, install, slots back ("${r.status}")`);
}
{
  const fw = { [NOW]: NEW, [NEW_PKG]: MOVED };
  const r = await page(packageOf(NEW_PKG, { map: MOVED.map }), NEW_PKG, fw, NOW, [[0, IMG[0]], [1, IMG[1]]], { failWriteAt: [1, 0x4200] });
  ok(/not written back/.test(r.status) && /Restore/.test(r.status) && /x0x_slots\.py restore/.test(r.status)
    && r.files.length === 1 && r.status.endsWith(r.files[0].name) && same(readArchive(r.files[0].b).images[1], IMG[1]),
    "installer page: failure mid-restore keeps the file and says how to restore it");
}

console.log(bad ? `INSTALL SLOT WEB TESTS FAILED (${bad})` : "install slot web tests passed");
process.exit(bad ? 1 : 0);
