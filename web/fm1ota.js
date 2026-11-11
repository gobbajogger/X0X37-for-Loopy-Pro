// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
//
// The FM-1 USB-MIDI update protocol (M-UPGRADE compatible) over WebMIDI.
// Step 1: the running firmware reads parts of the package and stages the update
// loader. Step 2: the loader reads the whole image and writes it. In both steps
// the device sends cmd 0x30 read requests on the logical image and we answer.

const HS_QUERY = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];
const MAXDATA = 512;
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const IS_OTA = (id) => /^ota-/i.test(id.model);
// the SMK-37 Pro, SMK-37 Elite, MKE-P37, Starrykey 37 Play and the SMK-37+VA mod: one keyboard under several model
// names, so a package for one runs on all of them; an FM-1 is another device
const SMK_FAMILY = /^(ota-)?(SMK-37[ +].*|MKE-P37|STARRYKEY-37.*)$/i;
export const sameKeyboard = (a, b) => a === b || (SMK_FAMILY.test(a) && SMK_FAMILY.test(b));   // two models

// errors carry a code the pages translate: notfound, model, stopped, noloader, lost, noreturn,
// mismatch (detail: the identity the device reports), badreq
function fail(code, msg, detail = "") { const e = new Error(msg); e.code = code; e.detail = detail; return e; }

export function pack7(data) {
  const out = []; let acc = 0, nb = 0;
  for (const b of data) {
    acc |= b << nb; nb += 8;
    while (nb >= 7) { out.push(acc & 0x7F); acc >>>= 7; nb -= 7; }
  }
  if (nb) out.push(acc & 0x7F);
  return out;
}

export function unpack7(s) {
  const out = []; let acc = 0, nb = 0;
  for (const b of s) {
    acc |= (b & 0x7F) << nb; nb += 7;
    while (nb >= 8) { out.push(acc & 0xFF); acc >>>= 8; nb -= 8; }
  }
  return out;
}

function response(addr, data, fl = 0) {
  const len = data.length;
  const body = [0x00, 0x59, 0x30, (len + 8) & 0xFF, ((len + 8) >> 8) & 0xFF, 0, fl,
    addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF,
    len & 0xFF, (len >> 8) & 0xFF, 0, ...data];
  let s = 0;
  for (let i = 6; i < body.length; i++) s += body[i];
  body.push(~s & 0xFF);
  return [0xF0, ...pack7(body), 0xF7];
}

function parseRequest(pkt) {
  if (pkt[0] !== 0xF0 || pkt[pkt.length - 1] !== 0xF7) return null;
  const wire = pkt.slice(1, -1);
  if (wire.some((b) => b & 0x80)) return null;       // SysEx data must be seven-bit
  const u = unpack7(wire);
  // ota_read sends an eight-byte body: flags, address32, requested length24.
  if (u.length !== 15 || u[0] !== 0 || u[1] !== 0x59 || u[2] !== 0x30 ||
      u[3] !== 8 || u[4] !== 0 || u[5] !== 0) return null;
  // Reject extra packed padding bits (non-canonical frames).
  if (pack7(u).some((b, i) => b !== wire[i])) return null;
  let s = 0;
  for (let i = 6; i < 14; i++) s += u[i];
  if ((~s & 0xFF) !== u[14]) return null;
  return { fl: u[6], addr: (u[7] | (u[8] << 8) | (u[9] << 16) | (u[10] << 24)) >>> 0, len: u[11] | (u[12] << 8) | (u[13] << 16) };
}

function parseIdentity(pkt) {
  if (pkt[0] !== 0xF0 || pkt[pkt.length - 1] !== 0xF7) return null;
  const wire = pkt.slice(1, -1);
  if (wire.some((b) => b & 0x80)) return null;       // SysEx data must be seven-bit
  const d = unpack7(wire);
  // ota_send_msg(0x11): 27 identity bytes, a 24-bit declared length, then ~sum(payload).
  if (d.length !== 34 || d[0] !== 0 || d[1] !== 0x59 || d[2] !== 0x11 ||
      d[3] !== 27 || d[4] !== 0 || d[5] !== 0) return null;
  let sum = 0;
  for (let i = 6; i < 33; i++) sum += d[i];
  if ((~sum & 0xFF) !== d[33]) return null;
  // Reject extra packed padding bits as well as status bytes hidden in the payload.
  if (pack7(d).some((b, i) => b !== wire[i])) return null;
  const txt = String.fromCharCode(...d.slice(6, 33)).replace(/\0+$/, "");
  const m = /^([^_]+)_(\d+)$/.exec(txt);
  return m ? { text: txt, model: m[1], version: parseInt(m[2], 10) } : null;
}

// one MIDI in/out pair with a SysEx queue
class Link {
  constructor(input, output) {
    this.input = input; this.output = output; this.q = []; this.waiter = null;
    this.rxBytes = 0; this.rxError = null;
    this.onMessage = (ev) => {
      if (this.rxError || ev.data[0] !== 0xF0) return;
      // Bound both frame count and bytes; overflow poisons this connection.
      // Never continue an OTA exchange with dropped or stale responses.
      // Receive bound per link: RX_FRAMES = 64, RX_BYTES = 65536 queued before the reader consumes them.
      if (this.q.length >= 64 || ev.data.length > 65536 - this.rxBytes) {
        this.rxError = Object.assign(new Error("MIDI receive overflow. Reconnect before retrying the update."), { code: "overflow", detail: "" });
        this.q.length = 0; this.rxBytes = 0;
        if (this.waiter) { const w = this.waiter; this.waiter = null; w(); }
        return;
      }
      this.rxBytes += ev.data.length;
      this.q.push(Array.from(ev.data));
      if (this.waiter) { const w = this.waiter; this.waiter = null; w(); }
    };
    // A MIDI input may already have an application handler. Do not replace it.
    if (typeof input.addEventListener === "function" &&
        typeof input.removeEventListener === "function") {
      this.useEvents = true;
      input.addEventListener("midimessage", this.onMessage);
    } else {
      // Compatibility with legacy/mock inputs that only expose the property.
      this.useEvents = false;
      const previous = input.onmidimessage;
      this.previousHandler = previous;
      this.installedHandler = (ev) => {
        this.onMessage(ev);
        if (typeof previous === "function") previous.call(input, ev);
      };
      input.onmidimessage = this.installedHandler;
    }
  }
  get lost() { return this.input.state === "disconnected" || this.output.state === "disconnected"; }
  // false when the port is gone (unplugged): WebMIDI throws on a disconnected output
  send(bytes) {
    try { this.output.send(bytes); return true; } catch (_) { return false; }
  }
  drain() { this.q.length = 0; this.rxBytes = 0; }
  async read(ms) {
    if (this.rxError) throw this.rxError;
    if (!this.q.length) {
      let tm;
      await Promise.race([new Promise((r) => { this.waiter = r; }), new Promise((r) => { tm = setTimeout(r, ms); })]);
      clearTimeout(tm);
      this.waiter = null;
    }
    if (this.rxError) throw this.rxError;
    const pkt = this.q.shift();
    if (pkt) this.rxBytes -= pkt.length;
    return pkt || null;
  }
  close() {
    if (this.useEvents) this.input.removeEventListener("midimessage", this.onMessage);
    else if (this.input.onmidimessage === this.installedHandler)
      this.input.onmidimessage = this.previousHandler;
  }
}

async function handshake(link, tries = 3) {
  link.drain();
  for (let t = 0; t < tries; t++) {
    if (!link.send(HS_QUERY)) return null;
    const end = Date.now() + 1000;
    while (Date.now() < end) {
      const p = await link.read(end - Date.now());
      if (!p) break;
      const id = parseIdentity(p);
      if (id) return id;
    }
  }
  return null;
}

export class Updater {
  constructor(access, log) {
    this.access = access;
    this.log = log || (() => {});
    // Port accepted by the last fail-closed transition wait (loader or restart).
    this.returnedPort = null;
    // Running ports observed while the selected unit is still in the loader (before reboot).
    this._beforeRunning = null;
  }

  _portPairs() {
    const ins = [...this.access.inputs.values()].filter((p) => p.state !== "disconnected");
    const outs = [...this.access.outputs.values()].filter((p) => p.state !== "disconnected");
    const pairs = [];
    for (const input of ins) {
      // No name gate: USB names change between firmwares. Only the handshake identity decides.
      // Names alone do not prove physical identity. Refuse duplicate or
      // mismatched port labels instead of guessing which keyboard receives MIDI.
      if (ins.filter((p) => p.name === input.name).length !== 1) continue;
      const matching = outs.filter((p) => p.name === input.name);
      if (matching.length !== 1) continue;
      pairs.push({ input, output: matching[0], name: input.name });
    }
    return pairs;
  }

  // every in/out pair with the same port name that answers the handshake
  async find(filter) {
    for (const { input, output, name } of this._portPairs()) {
      try { await input.open(); await output.open(); } catch (_) { continue; }
      const link = new Link(input, output);
      const id = await handshake(link, 2);
      if (id && (!filter || filter(id))) return { link, id, name };
      link.close();
    }
    return null;
  }

  // Port names that answer and match filter (every link closed). Used to snapshot before a transition.
  // skip: a port name already held open; do not reopen it (would steal onmidimessage from the live link).
  async matchingPorts(filter, skip = null) {
    const names = [];
    for (const { input, output, name } of this._portPairs()) {
      if (skip != null && name === skip) continue;
      try { await input.open(); await output.open(); } catch (_) { continue; }
      const link = new Link(input, output);
      try {
        const id = await handshake(link, 2);
        if (id && (!filter || filter(id))) names.push(name);
      } finally { link.close(); }
    }
    return names;
  }

  async openPort(name, filter) {
    const pair = this._portPairs().find((p) => p.name === name);
    if (!pair) return null;
    try { await pair.input.open(); await pair.output.open(); } catch (_) { return null; }
    const link = new Link(pair.input, pair.output);
    const id = await handshake(link, 2);
    if (id && (!filter || filter(id))) return { link, id, name };
    link.close();
    return null;
  }

  // Fail-closed transition wait: accept exactly one newly appeared matching port.
  // before: port names that already matched filter before the transition.
  // Returns that device, or null when none appear before the timeout.
  // Throws when more than one new matching port appears. Never returns a port in before.
  async waitFor(filter, ms, before) {
    const end = Date.now() + ms;
    const prior = new Set(before);
    while (Date.now() < end) {
      const names = await this.matchingPorts(filter);
      const newcomers = names.filter((n) => !prior.has(n));
      if (newcomers.length > 1) {
        throw fail("model", "more than one new device appeared after the update transition; disconnect other keyboards and try again. Nothing was written");
      }
      if (newcomers.length === 1) {
        const dev = await this.openPort(newcomers[0], filter);
        if (dev) { this.returnedPort = newcomers[0]; return dev; }
      }
      await sleep(1000);
    }
    return null;
  }

  // Reconnect after a verified transition: same newly-appeared rule vs other ports.
  async reconnectAfter(filter, ms) {
    const known = this.returnedPort;
    const before = (await this.matchingPorts(filter)).filter((n) => n !== known);
    return this.waitFor(filter, ms, before);
  }

  // serve read requests until the device asks for one of the finish addresses; stops early
  // when the port disappears (lost: true) instead of waiting out idleMs
  async serve(link, image, finishAddr, idleMs, onProgress) {
    let served = 0, last = Date.now();
    for (;;) {
      const pkt = await link.read(1000);
      if (!pkt) {
        if (link.lost) return { served, finished: false, lost: true };
        if (Date.now() - last > idleMs) return { served, finished: false };
        continue;
      }
      const r = parseRequest(pkt);
      if (!r) continue;
      last = Date.now();
      if (r.addr === 0xE0000000 || r.addr === 0xF0000000) {
        // Terminal app (0xE0000000) and loader (0xF0000000) reads ask for exactly the
        // eight-byte "success\0" reply (ota_read(addr, b, 8) in firmware/src/ota.c and the loader).
        if (r.len !== 8) throw fail("badreq", `bad finish request ${r.addr.toString(16)}+${r.len}`);
        if (!link.send(response(r.addr, [...new TextEncoder().encode("success"), 0]))) {
          return { served, finished: false, lost: true };
        }
        if (r.addr === finishAddr) return { served, finished: true };
        continue;
      }
      // A zero-length ordinary read is never valid: it would stall progress silently.
      if (r.len === 0 || r.len > MAXDATA || r.addr + r.len > image.length) throw fail("badreq", `bad request ${r.addr.toString(16)}+${r.len}`);
      await sleep(10);
      if (!link.send(response(r.addr, Array.from(image.subarray(r.addr, r.addr + r.len)), r.fl))) {
        return { served, finished: false, lost: true };
      }
      served++;
      if (onProgress) onProgress(served, r.addr);
    }
  }

  // full install: running firmware -> loader -> new firmware
  async install(image, product, onStep, { beforeUpgrade } = {}) {
    const step = onStep || (() => {});
    const dev = await this.find((id) => !IS_OTA(id));
    if (!dev) throw fail("notfound", "FM-1 not found (USB cable, and no other app using it?)");
    const [model] = product.split("_");
    if (!sameKeyboard(dev.id.model, model)) { dev.link.close(); throw fail("model", `the device is ${dev.id.text}, the package is for ${product}`); }
    const beforeLoaders = await this.matchingPorts(IS_OTA, dev.name);
    try {
      if (beforeUpgrade) await beforeUpgrade(dev);
      if (dev.link.lost) throw fail("lost", "The keyboard connection changed before OTA. Nothing was written.");
    } catch (e) { dev.link.close(); throw e; }
    step("start", dev.id.text);
    if (!dev.link.send(UPGRADE)) { dev.link.close(); throw fail("lost", "The USB connection was lost before OTA."); }
    await sleep(2000);
    const s1 = await this.serve(dev.link, image, 0xE0000000, 8000, (n) => step("verify", n));
    dev.link.close();
    if (!s1.finished) throw fail(s1.lost ? "lost" : "stopped", `the device ${s1.lost ? "was disconnected" : "stopped"} after ${s1.served} requests: nothing was written`);
    step("loader");
    await sleep(3000);
    const ota = await this.waitFor((id) =>
      IS_OTA(id) && sameKeyboard(id.model.replace(/^ota-/i, ""), model), 30000, beforeLoaders);
    if (!ota) throw fail("noloader", "the update loader did not appear. Replug the USB cable and press Install again: the device stays in update mode until it is finished.");
    // Reject a loader from another keyboard family, even if discovery changes.
    if (!IS_OTA(ota.id) || !sameKeyboard(ota.id.model.replace(/^ota-/i, ""), model)) {
      ota.link.close();
      throw fail("model", `the loader is ${ota.id.text}, the package is for ${product}`, ota.id.text);
    }
    const s2 = await this.write(ota, image, step);
    if (!s2.finished) throw fail(s2.lost ? "lost" : "stopped", `the loader ${s2.lost ? "was disconnected" : "stopped"} after ${s2.served} requests. Replug and press Install again to resume.`);
    const beforeRunning = this._beforeRunning || [];
    step("reboot");
    await sleep(3000);
    // A different connected keyboard is not evidence that the target rebooted.
    // In particular, stock/manual mode must not report false success on mismatch.
    const back = await this.waitFor((id) => !IS_OTA(id) && sameKeyboard(id.model, model), 40000, beforeRunning);
    if (!back || !sameKeyboard(back.id.model, model))
      throw fail("noreturn", "the target keyboard did not come back: power-cycle it");
    back.link.close();
    if (back.id.text !== product) throw fail("mismatch", `installed, but the device reports ${back.id.text}`, back.id.text);
    step("done", back.id.text);
    return back.id.text;
  }

  // step 2 with the loader found by find()/waitFor()
  async write(ota, image, step) {
    // Snapshot other running keyboards while this unit is still in the loader.
    this._beforeRunning = await this.matchingPorts((id) => !IS_OTA(id), ota.name);
    ota.link.send(UPGRADE);
    await sleep(2000);
    const s2 = await this.serve(ota.link, image, 0xF0000000, 180000, (n) => step("write", n));
    ota.link.close();
    return s2;
  }

  // resume: the device is already in update mode (loader) -> true when the write finished. product: the
  // package's identity; the loader's model must match it by install()'s rule (sameKeyboard): an SMK's loader
  // never gets an FM-1 image, and without the identity nothing is written
  async resume(image, onStep, product = "") {
    const ota = await this.find(IS_OTA);
    if (!ota) return false;
    if (!product || !sameKeyboard(ota.id.model.replace(/^ota-/i, ""), product.split("_")[0])) {
      ota.link.close();
      throw fail("model", `the device is in the update mode of ${ota.id.text}, the package is for ${product || "an unknown model"}`, ota.id.text);
    }
    return (await this.write(ota, image, onStep || (() => {}))).finished;
  }
}

// Test-only handle for the bounded receive queue (web/test_ota_request_guards.mjs).
export const __test_Link = Link;
