// SPDX-License-Identifier: GPL-3.0-only
// F03: installKeepingSlots must fail closed when the sample-slot layout probe fails.
// No real MIDI, no device write. Models today's silent skip (.catch(() => null) then install)
// and asserts the repaired rule. Run from the repo root: node web/test_f03_slot_probe.mjs
import { Updater, pack7, sameKeyboard } from "./fm1ota.js";
import { installKeepingSlots, packageSlotMap } from "./x0x_breaks.js";

let bad = 0;
const ok = (c, what) => { console.log(`${what.padEnd(72)} ${c ? "ok" : "FAIL"}`); if (!c) bad++; };
const HS = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];
const eq = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
const PRODUCT = "SMK-37 Pro_900";

function packageOf(product) {
  const raw = Uint8Array.from({ length: 0x2000 + 20 }, (_, i) => (i * 7) & 0xFF);
  for (let i = 0; i < 20; i++) raw[i * 0x30 + 0x2F] = i < product.length ? (product.charCodeAt(i) + i + 1) & 0xFF : 0x7D;
  raw.set([..."FELUCCA-LOADER-1"].map((c) => c.charCodeAt(0)), 0x1800);
  const logical = [];
  for (let i = 0; i < 20; i++) logical.push(...raw.subarray(i * 0x30, i * 0x30 + 0x2F));
  return { raw, image: Uint8Array.from([...logical, ...raw.subarray(20 * 0x30)]) };
}

/** Simulated SMK: answers identity and the update protocol; slot probe is controlled. */
class FakeSMK {
  constructor(image, { answerSlots = "empty" } = {}) {
    this.image = image;
    this.answerSlots = answerSlots; // "empty" | "mute" | "bad"
    this.upgrades = 0;
    this.access = { inputs: new Map(), outputs: new Map() };
    this.boot(PRODUCT, "SMK-37 Pro"); // Current X0X must answer SMP_INFO; stock _016 has a separate exemption.
  }
  boot(identity, name) {
    this.identity = identity;
    this.waiting = null;
    this.queue = [];
    for (const m of [this.access.inputs, this.access.outputs]) {
      for (const p of m.values()) p.state = "disconnected";
      m.clear();
    }
    const id = Math.random().toString(36).slice(2);
    this.input = { id: "i" + id, name, state: "connected", onmidimessage: null, open: async () => {} };
    this.output = {
      id: "o" + id, name, state: "connected", open: async () => {},
      send: (d) => {
        if (this.output.state !== "connected") throw new Error("InvalidStateError");
        setImmediate(() => this.rx(Array.from(d)));
      },
    };
    this.access.inputs.set(this.input.id, this.input);
    this.access.outputs.set(this.output.id, this.output);
  }
  tx(bytes) {
    const i = this.input;
    setImmediate(() => { if (i.state === "connected" && i.onmidimessage) i.onmidimessage({ data: Uint8Array.from(bytes) }); });
  }
  rx(d) {
    if (d[1] === 0x7D && d[2] === 0x46 && d[3] === 0x4C) {
      if (this.answerSlots === "mute") return;
      if (d[4] === 15) {
        if (this.answerSlots === "bad") {
          this.tx([0xF0, 0x7D, 0x46, 0x4C, 15, 0xF7]); // short / unparsable
          return;
        }
        // Valid empty layout: one 80 KiB slot with zero zones, empty name and zero used KiB.
        this.tx([0xF0, 0x7D, 0x46, 0x4C, 15, 1, 80, 0, 0, 0, 0xF7]);
      }
      return;
    }
    if (eq(d, HS)) {
      const t = [...new TextEncoder().encode(this.identity)];
      const payload = [...t, ...new Array(27 - t.length).fill(0)];
      const body = [0, 0x59, 0x11, 27, 0, 0, ...payload, ~payload.reduce((sum, b) => sum + b, 0) & 255];
      this.tx([0xF0, ...pack7(body), 0xF7]);
    } else if (eq(d, UPGRADE)) {
      this.upgrades++;
      this.queue = this.identity.startsWith("ota-")
        ? [...Array.from({ length: 6 }, (_, k) => [k * 512, 512]), [0xF0000000, 8]]
        : [[0, 64], [0x40, 160], [0x1000, 512], [0xE0000000, 8]];
      this.next();
    } else if (this.waiting) {
      const u = (() => {
        const out = [];
        const a = d.slice(1, -1);
        for (let i = 0; i < a.length;) {
          const m = a[i++];
          for (let j = 0; j < 7 && i < a.length; j++) out.push(a[i++] | ((m >> j) & 1) << 7);
        }
        return out;
      })();
      const [addr, len] = this.waiting;
      const want = addr >= 0xE0000000 ? [...new TextEncoder().encode("success"), 0]
        : Array.from(this.image.subarray(addr, addr + len));
      void u; void want;
      this.waiting = null;
      if (addr === 0xE0000000) setTimeout(() => this.boot("ota-SMK-37 Pro_900", "Felucca Update"), 20);
      else if (addr === 0xF0000000) setTimeout(() => this.boot(PRODUCT, "SMK-37 Pro"), 20);
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

/** Pre-repair twin: .catch(() => null) then install anyway. */
async function silentSkipInstall(up, image, product, board, hooks) {
  const h = Object.assign({ step() {}, say() {}, save() {}, progress() {}, sameKeyboard: () => true }, hooks);
  const fail = (code, msg, detail = "") => Object.assign(new Error(msg), { code, detail });
  const { readSlotLayout } = await import("./x0x_breaks.js");
  const dev = await up.find((id) => !/^ota-/i.test(id.model));
  if (dev && !h.sameKeyboard(dev.id.model, product.split("_")[0])) dev.link.close();
  else if (dev) {
    try {
      const layout = await readSlotLayout(dev.link, 200, 1).catch(() => null);
      if (layout && layout.slots.some((sl) => sl.nz)) {
        throw fail("unexpected", "silent-skip model expected empty/null layout");
      }
    } finally { dev.link.close(); }
  }
  return up.install(image, product, h.step);
}

async function main() {
  const pkg = packageOf(PRODUCT);
  ok(packageSlotMap(pkg.image).length >= 0, "fixture package loads");

  // Model: mute probe + silent skip still installs (today's bug).
  {
    const dev = new FakeSMK(pkg.image, { answerSlots: "mute" });
    const ran = await silentSkipInstall(new Updater(dev.access), pkg.image, PRODUCT, "smk", { sameKeyboard });
    ok(ran === PRODUCT && dev.upgrades === 2,
       "model: pre-repair mute probe still installs (silent skip)");
  }

  // Repaired: mute probe fails closed before write.
  {
    const dev = new FakeSMK(pkg.image, { answerSlots: "mute" });
    const err = await installKeepingSlots(new Updater(dev.access), pkg.image, PRODUCT, "smk", {
      sameKeyboard,
    }).then(() => null, (e) => e);
    ok(err && err.code === "slotlayout" && /Cannot verify the current sample layout before install/.test(err.message)
       && dev.upgrades === 0,
       "installKeepingSlots: mute probe -> slotlayout error, nothing written");
  }

  // Successful empty layout still installs.
  {
    const dev = new FakeSMK(pkg.image, { answerSlots: "empty" });
    const res = await installKeepingSlots(new Updater(dev.access), pkg.image, PRODUCT, "smk", {
      sameKeyboard,
    });
    ok(res && res.product === PRODUCT && res.move === null && dev.upgrades === 2,
       "installKeepingSlots: successful empty layout installs without backup");
  }

  // Unparsable layout also fails closed.
  {
    const dev = new FakeSMK(pkg.image, { answerSlots: "bad" });
    const err = await installKeepingSlots(new Updater(dev.access), pkg.image, PRODUCT, "smk", {
      sameKeyboard,
    }).then(() => null, (e) => e);
    ok(err && err.code === "slotlayout" && /short SMP_INFO reply/.test(err.message) && dev.upgrades === 0,
       "installKeepingSlots: bad layout parse -> slotlayout error, nothing written");
  }

  process.exit(bad ? 1 : 0);
}

main().catch((e) => { console.error(e); process.exit(1); });
