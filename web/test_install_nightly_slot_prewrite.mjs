// SPDX-License-Identifier: GPL-3.0-only
// Exact browser-helper prewrite regression; pure in-memory SysEx, no Web MIDI/device.
import { parseInfo, installKeepingSlots } from "./x0x_breaks.js";

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
