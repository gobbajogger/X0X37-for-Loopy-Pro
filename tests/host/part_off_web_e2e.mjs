// SPDX-License-Identifier: GPL-3.0-only
// M7a end to end: the firmware host build's exported .x0x project files (tests/host/part_off_web_e2e.c, the
// real project_file_header() and x0x.h PROJ_FILE_OFF_WORD) through the real web parseProject().
// Usage: node tests/host/part_off_web_e2e.mjs <dir of mask0..31.x0xp>. Output "  PASS|FAIL <case>".
import { readFileSync } from "node:fs";
import { parseProject } from "../../web/x0x_projects.js";

const dir = process.argv[2];
if (!dir) { console.error("usage: part_off_web_e2e.mjs <dir>"); process.exit(2); }
let fails = 0;
const verdict = (name, ok, why = "") => { console.log(`  ${ok ? "PASS" : "FAIL"} ${name}${ok || !why ? "" : "  (" + why + ")"}`); if (!ok) fails++; };
const load = m => new Uint8Array(readFileSync(`${dir}/mask${m}.x0xp`));
const w28 = b => new DataView(b.buffer, b.byteOffset).getUint32(28, true);
const set28 = (b, w) => { const c = b.slice(); new DataView(c.buffer).setUint32(28, w >>> 0, true); return c; };
const parse = b => { try { return { ok: true, r: parseProject(b) }; } catch (e) { return { ok: false, code: e.code, msg: e.message }; } };

// 1. all ON: word 28 is 0, accepted, mask 0
{ const b = load(0), p = parse(b);
  verdict("fw_all_on_word28_zero_accepted_mask0", w28(b) === 0 && p.ok && p.r.partOffMask === 0, JSON.stringify(p)); }
// 2. every 1..31 OFF mask the firmware exports: accepted with that partOffMask
{ const bad = [];
  for (let m = 1; m < 32; m++) {
    const b = load(m), w = w28(b), p = parse(b);
    if ((w & 0xFFFF0000) >>> 0 !== 0x4F460000 || (w & 0xFF) !== m || !p.ok || p.r.partOffMask !== m)
      bad.push(`m${m} w=${w.toString(16)} ${p.ok ? "mask " + p.r.partOffMask : p.code + ":" + p.msg}`);
  }
  verdict("fw_all_31_off_masks_accepted_exact_mask", bad.length === 0, bad.slice(0, 4).join("; ")); }
// 3. firmware's word with a wrong check byte (each of the 255 others, every mask): rejected, code "version"
{ const bad = [];
  for (let m = 1; m < 32; m++) {
    const b = load(m), w = w28(b);
    for (let x = 1; x < 256; x++) {
      const p = parse(set28(b, w ^ (x << 8)));
      if (p.ok || p.code !== "version") { bad.push(`m${m} x${x} ${p.ok ? "accepted mask " + p.r.partOffMask : p.code}`); break; }
    }
  }
  verdict("fw_word28_wrong_check_byte_rejected_version", bad.length === 0, bad.slice(0, 4).join("; ")); }
// 4. one flipped mask bit with the firmware's check byte kept: rejected (never a different mask)
{ const bad = [];
  for (let m = 1; m < 32; m++) {
    const b = load(m), w = w28(b);
    for (let i = 0; i < 5; i++) {
      const n = m ^ (1 << i), p = parse(set28(b, n ? (w & ~0xFF) | n : (w & ~0xFF))); // n 0 keeps tag+check: invalid
      if (p.ok || p.code !== "version") bad.push(`m${m}->${n} ${p.ok ? "accepted mask " + p.r.partOffMask : p.code}`);
    }
  }
  verdict("fw_word28_flipped_mask_bit_rejected", bad.length === 0, bad.slice(0, 4).join("; ")); }
// 5. control: the firmware's file with only word 28 zeroed is still the valid all-ON file
{ const p = parse(set28(load(5), 0)); verdict("fw_mask5_word28_zeroed_accepted_mask0", p.ok && p.r.partOffMask === 0, JSON.stringify(p)); }
console.log(`part_off_web_e2e: ${fails ? "FAIL" : "ok"} (${fails} failing cases)`);
process.exit(fails ? 1 : 0);
