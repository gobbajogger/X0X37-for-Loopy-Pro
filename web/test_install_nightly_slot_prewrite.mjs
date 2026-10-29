// SPDX-License-Identifier: GPL-3.0-only
// Exact browser-helper prewrite regression; pure in-memory SysEx, no Web MIDI/device.
import { parseInfo, installKeepingSlots, makeArchive, readArchive, pack78, crc32 } from "./x0x_breaks.js";

let checks = 0;
function ok(value, label) {
  if (!value) throw new Error("SLOT PREWRITE FAIL: " + label);
  checks++;
}
function throwsInfo(reply) {
  try { parseInfo(Uint8Array.from(reply)); return false; }
  catch (_) { return true; }
}
const emptyLegacy = [1, 80, 0, 0, 0];
const emptyModern = [1, 80, 0, 0, 0, 0x58, 1, 0x42, 1, 20, 0, 0, 0];
ok(parseInfo(Uint8Array.from(emptyLegacy)).ext === false, "complete legacy X0X layout");
ok(parseInfo(Uint8Array.from(emptyModern)).ext === true, "complete modern X0X layout");
ok(parseInfo(Uint8Array.from(emptyModern)).slots[0].off === 0xC2000, "extended offset");
for (const [label, data] of [
  ["empty reply", []],
  ["short reply", [1]],
  ["zero slots", [0, 80]],
  ["bad minimum size", [1, 0, 0, 0, 0]],
  ["incomplete name", [1, 80, 0, 65, 66]],
  ["overlong name", [1, 80, 0, 65, 65, 65, 65, 65, 65, 65, 65, 65, 0, 0]],
  ["high-bit used-KiB", [1, 80, 0, 0, 200]],
  ["trailing junk", [1, 80, 0, 0, 0, 7]],
  ["incomplete extension", [1, 80, 0, 0, 0, 0x58, 1, 0]],
  ["unknown extension version", [1, 80, 0, 0, 0, 0x58, 2, 0, 0, 0, 0, 0, 0]],
  ["zero extended size", [1, 80, 0, 0, 0, 0x58, 1, 0x42, 1, 0, 0, 0, 0]],
]) ok(throwsInfo(data), "reject " + label);

async function attempt(identity, reply) {
  let installs = 0, closes = 0, commands = 0;
  const link = {
    drain() {},
    send(pkt) { if (pkt[4] === 15) commands++; return true; },
    async read() { return reply === null ? null : [0xF0, 0x7D, 0x46, 0x4C, 15, ...reply, 0xF7]; },
    close() { closes++; },
  };
  const up = {
    async find() { return { link, id: { model: identity.split("_")[0], text: identity } }; },
    async install() { installs++; return "synthetic-installed"; },
  };
  let error = null;
  try { await installKeepingSlots(up, new Uint8Array(16), "SMK-37 Pro_9001100",
    "smk", { sameKeyboard: () => true }); }
  catch (e) { error = e; }
  return { installs, closes, commands, error };
}
for (const [label, id, bytes, calls, errorCode] of [
  ["modern good", "SMK-37 Pro_9001050", emptyModern, 1, null],
  ["legacy X0X good", "SMK-37 Pro_9001000", emptyLegacy, 1, null],
  ["modern short", "SMK-37 Pro_9001050", [1], 0, "slotlayout"],
  ["modern no reply", "SMK-37 Pro_9001050", null, 0, "slotlayout"],
  ["modern bad extension", "SMK-37 Pro_9001050", [1, 80, 0, 0, 0, 0x58, 1, 0], 0, "slotlayout"],
  ["stock Pro 016 no SMP_INFO", "SMK-37 Pro_016", null, 1, null],
  ["stock Pro 016 damaged response", "SMK-37 Pro_016", [1], 0, "slotlayout"],
  ["unclassified 017 no reply", "SMK-37 Pro_017", null, 0, "slotlayout"],
]) {
  const r = await attempt(id, bytes);
  ok(r.installs === calls, label + ": expected install count");
  ok((r.error && r.error.code || null) === errorCode, label + ": expected outcome");
  ok(r.closes === 1 && r.commands > 0, label + ": closed link after probe");
}
if (typeof process !== "undefined") console.log("installer SMP_INFO prewrite checked: " + checks + "/" + checks);


// ---- An SMP_INFO-occupied source slot must never become an "empty" backup.
//      Exercise real readSlotImage/backupSlots/installKeepingSlots functions
//      over synthetic slot MIDI replies; no firmware or hardware writes. ----
function occupiedScenario(model, mode) {
  const samples = Uint8Array.from([3, 4, 5, 6]);
  const layout = mode === "empty" ? [1, 80, 0, 0, 0] : [1, 80, 1, 0, 1];
  const hdr = new Uint8Array(512), view = new DataView(hdr.buffer);
  view.setUint32(0, 0x504D5346, true);
  view.setUint16(4, mode === "version" ? 2 : 1, true);
  hdr[6] = mode === "nz" ? 2 : 1;
  view.setUint32(16, mode === "oversize" ? 90000 : mode === "zero" ? 0 : samples.length, true);
  view.setUint32(20, (crc32(samples) + (mode === "crc" ? 1 : 0)) >>> 0, true);
  view.setUint32(32, 0, true);
  view.setUint32(36, samples.length, true);
  view.setUint32(48, 65536, true);
  if (mode === "magic") view.setUint32(0, 0, true);
  let last = null, installs = 0, closes = 0, saved = null;
  const link = {
    drain() {}, send(msg) { last = msg; return true; },
    async read() {
      const cmd = last[4];
      if (cmd === 15) return [0xF0, 0x7D, 0x46, 0x4C, 15, ...layout, 0xF7];
      if (cmd === 16) {
        const addr = last[6] | last[7] << 7 | last[8] << 14;
        const len = 2 * last[9];
        const bytes = new Uint8Array(512 + samples.length);
        bytes.set(hdr); bytes.set(samples, 512);
        const part = addr < bytes.length ? bytes.subarray(addr, addr + len) : new Uint8Array(len);
        return [0xF0, 0x7D, 0x46, 0x4C, 16, ...last.slice(5, 9), ...pack78(part), 0xF7];
      }
      throw Error("unexpected synthetic slot command " + cmd);
    },
    close() { closes++; }
  };
  const up = {
    async find() { return { link, id: { model, text: model + "_9001100" } }; },
    async install() { installs++; return "simulated-installed"; }
  };
  return {
    async execute() {
      let error = null, move = null;
      try {
        const r = await installKeepingSlots(up, new Uint8Array(16), model + "_9001100",
          "smk", { sameKeyboard: () => true, save: bytes => { saved = bytes; } });
        move = r.move;
      } catch (e) { error = e; }
      return { installs, closes, error, move,
        archive: saved ? readArchive(saved) : null };
    }
  };
}
for (const model of ["SMK-37 Elite", "SMK-37 Pro"]) {
  for (const mode of ["empty", "valid", "magic", "version", "nz", "oversize", "zero", "crc"]) {
    const r = await occupiedScenario(model, mode).execute();
    const expectedWrite = mode === "empty" || mode === "valid";
    ok(r.installs === (expectedWrite ? 1 : 0),
      model + " " + mode + ": failed occupied backup must prevent firmware write");
    ok(r.closes === 1, model + " " + mode + ": slot probe link closed");
    if (mode === "valid") {
      ok(!!r.archive && r.archive.meta.slots[0].empty === false,
        model + " valid: archived occupied sample must be nonempty");
      ok(r.archive.images[0] !== null, model + " valid: sample payload present");
    } else if (mode !== "empty") {
      ok(!!r.error && r.error.code === "slotbackup",
        model + " " + mode + ": explicit prewrite backup failure");
      ok(r.archive === null, model + " " + mode + ": no misleading archive created");
    }
  }
}
for (const model of ["SMK-37 Elite", "SMK-37 Pro"]) {
  let rejected = false;
  try {
    makeArchive({ count: 1, size: 81920, slots: [{ nz: 1, size: 81920 }] },
      [null], "2026-10-24T00:00:00");
  } catch (e) { rejected = /occupied source slot/.test(String(e.message)); }
  ok(rejected, model + ": archive creation must refuse occupied null image");
}
console.log("installer occupied sample header backup checked: " + checks + "/" + checks);
