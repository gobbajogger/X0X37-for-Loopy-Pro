// SPDX-License-Identifier: GPL-3.0-only
// Boot screen packer and save replies. No device, no browser canvas.
import { readFileSync } from "node:fs";
import { bootPack, bootRgbaToBits, bootTransfer, BOOT_LINE0, BOOT_LINE1, BOOT_FIXED, BOOT_PIC_BYTES, BOOT_FILE_MAX } from "./boot_screen.js";

let bad = 0;
const check = (name, ok) => { console.log(name.padEnd(72) + (ok ? "ok" : "FAIL")); if (!ok) bad++; };

const factory = bootPack({ factory: true });
check("original-words record is 44 bytes, kind 2", factory.ok && factory.bytes.length === BOOT_FIXED && factory.bytes[0] === 2 &&
  factory.bytes.slice(1).every((b) => b === 0));
check("empty top line is refused", bootPack({ line0: "", line1: "HELLO" }).code === "short");
check("empty second line is refused", bootPack({ line0: "HI", line1: "" }).code === "short");
check("top line over 12 is refused", bootPack({ line0: "X".repeat(BOOT_LINE0 + 1), line1: "OK" }).code === "long0");
check("second line over 21 is refused", bootPack({ line0: "OK", line1: "Y".repeat(BOOT_LINE1 + 1) }).code === "long1");
check("a newline in the text is refused", bootPack({ line0: "HI", line1: "A\nB" }).code === "chars");
check("a picture of the wrong length is refused", bootPack({ line0: "HI", line1: "THERE", bits: new Uint8Array(10) }).code === "picsize");
const custom = bootPack({ line0: "MORN", line1: "SHIFT" });
check("text-only record keeps both lines and no picture", custom.ok && custom.bytes.length === BOOT_FIXED &&
  custom.bytes[0] === 1 && custom.bytes[1] === 0 && custom.bytes[2] === 4 && custom.bytes[3] === 5 &&
  String.fromCharCode(...custom.bytes.slice(4, 8)) === "MORN");
const bits = new Uint8Array(BOOT_PIC_BYTES);
bits[0] = 0x80;
const pictured = bootPack({ line0: "MORN", line1: "SHIFT", bits });
check("240 by 96 picture is stored after the header", pictured.ok && pictured.bytes.length === BOOT_FIXED + BOOT_PIC_BYTES &&
  pictured.bytes[1] === 1 && pictured.bytes[40] === 240 && pictured.bytes[42] === 96 && pictured.bytes[BOOT_FIXED] === 0x80);
check("a wrong-size pixel buffer is refused", bootRgbaToBits(new Uint8Array(4)) === null);
const rgba = new Uint8ClampedArray(240 * 96 * 4);
rgba[0] = rgba[1] = rgba[2] = rgba[3] = 255;
const one = bootRgbaToBits(rgba);
check("a white pixel becomes the top-left bit", one && one[0] === 0x80 && one[1] === 0);
check("import limit is 8 MB", BOOT_FILE_MAX === 8 * 1024 * 1024);

function frame(cmd, payload) { return [0xF0, 0x7D, 0x46, 0x4C, cmd, ...payload, 0xF7]; }
function scripted(replies) {
  const sent = [];
  let i = 0;
  return {
    sent,
    drain() {},
    send(bytes) { sent.push(Array.from(bytes)); return true; },
    async read() { return replies[i++] || null; },
  };
}
const okWrite = (off) => frame(44, [off & 127, (off >> 7) & 127, (off >> 14) & 127, 0]);

const saved = await bootTransfer(scripted([
  frame(43, [0]), okWrite(0), frame(46, [0]),
]), factory.bytes);
check("a confirmed save is the only success", saved.ok === true && saved.ended === true);

const retriedEnd = await bootTransfer(scripted([
  frame(43, [0]), okWrite(0), null, frame(46, [0]),
]), factory.bytes);
check("a lost end reply retried as success", retriedEnd.ok === true && retriedEnd.ended === true);

const refused = await bootTransfer(scripted([
  frame(43, [0]), okWrite(0), frame(46, [1]),
]), factory.bytes);
check("a refused record is not success", refused.ok === false && refused.code === 1 && refused.ended === true);

const flashLink = scripted([
  frame(43, [0]), frame(44, [0, 0, 0, 3]), frame(47, [0]),
]);
const full = await bootTransfer(flashLink, factory.bytes);
check("a flash error does not claim a save", full.ok === false && full.code === 3 && full.ended === false);
check("a flash error aborts the unfinished stage", flashLink.sent.some((b) => b[4] === 47));

const busy = await bootTransfer(scripted([frame(43, [4])]), factory.bytes);
check("a busy keyboard is not success and does not write", busy.ok === false && busy.code === 4);

const html = readFileSync(new URL("./x0x_installer.html", import.meta.url), "utf8");
for (const phrase of [
  "That file is empty. The boot screen was not changed.",
  "That file is not a picture this page can read. The boot screen was not changed.",
  "That picture is too big to import (over 8 MB). The boot screen was not changed.",
  "That picture could not be read. The boot screen was not changed.",
  "That picture has no pixels. The boot screen was not changed.",
  "The top line can be 12 characters. The boot screen was not changed.",
  "The second line can be 21 characters. The boot screen was not changed.",
  "Type both lines, or choose Original words. The boot screen was not changed.",
  "The keyboard refused this boot screen. The previous one is still there.",
  "The keyboard could not store this. The previous boot screen is still there.",
  "The save did not finish. The previous boot screen is still there.",
  "The keyboard did not confirm the save.",
  "Saved. The new boot screen shows the next time the keyboard starts.",
]) check("page says: " + phrase.slice(0, 42), html.includes(phrase));

console.log(bad ? "BOOT SCREEN WEB TEST FAILED" : "boot screen web test passed");
process.exit(bad ? 1 : 0);
