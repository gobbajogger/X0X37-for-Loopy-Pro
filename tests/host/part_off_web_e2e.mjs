// SPDX-License-Identifier: GPL-3.0-only
// M7a end to end: the firmware host build's exported .x0x project files (tests/host/part_off_web_e2e.c, the
// real project_file_header() and x0x.h PROJ_FILE_OFF_WORD) through the real web parseProject().
// Also cross-checks fw_validate.txt from the C harness: web parseProject must agree with
// project_file_validate on every recorded word (accept/reject).
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
const offChk = m => (m & 1 ? 0x17 : 0) ^ (m & 2 ? 0x2E : 0) ^ (m & 4 ? 0x5C : 0) ^ (m & 8 ? 0xB8 : 0) ^ (m & 16 ? 0x67 : 0);
const webWord = m => (m ? (0x4F460000 | (offChk(m) << 8) | m) >>> 0 : 0);

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

// 6. legacy 0x4F460000|mask (check byte 0) for masks 1..31: rejected
{ const bad = [];
  const b = load(0);
  for (let m = 1; m < 32; m++) {
    const p = parse(set28(b, (0x4F460000 | m) >>> 0));
    if (p.ok || p.code !== "version") bad.push(`m${m} ${p.ok ? "accepted" : p.code}`);
  }
  verdict("fw_legacy_tag_mask_rejected_version", bad.length === 0, bad.slice(0, 4).join("; ")); }

// 7. mask byte above 31 with a plausible check: rejected
{ const bad = [];
  const b = load(0);
  for (let m = 32; m < 256; m++) {
    const w = (0x4F460000 | (offChk(m & 31) << 8) | m) >>> 0;
    const p = parse(set28(b, w));
    if (p.ok || p.code !== "version") bad.push(`m${m} ${p.ok ? "accepted" : p.code}`);
  }
  verdict("fw_mask_byte_above_31_rejected_version", bad.length === 0, bad.slice(0, 4).join("; ")); }

// 8. 1-bit flips of all 32 bits of every web-built word: rejected
{ const bad = [];
  const b = load(0);
  for (let m = 0; m < 32; m++) {
    const w = webWord(m);
    for (let i = 0; i < 32; i++) {
      const p = parse(set28(b, (w ^ (1 << i)) >>> 0));
      if (p.ok || p.code !== "version") bad.push(`m${m} bit${i} ${p.ok ? "accepted" : p.code}`);
    }
  }
  verdict("fw_word28_1bit_flips_rejected_version", bad.length === 0, bad.slice(0, 4).join("; ")); }

// 9. Cross-check: for every word in fw_validate.txt, parseProject accept/reject must match project_file_validate
{ const lines = readFileSync(`${dir}/fw_validate.txt`, "utf8").trim().split(/\n/);
  const b = load(0);
  const disagree = [];
  for (const line of lines) {
    const [hex, expect] = line.trim().split(/\s+/);
    const w = Number.parseInt(hex, 16) >>> 0;
    const p = parse(set28(b, w));
    const webAccept = !!(p.ok);
    const fwAccept = expect === "accept";
    if (webAccept !== fwAccept)
      disagree.push(`${hex} fw=${expect} web=${webAccept ? "accept" : "reject" + (p.code ? ":" + p.code : "")}`);
  }
  verdict("web_parse_agrees_project_file_validate", disagree.length === 0,
    disagree.length ? `${disagree.length} disagree e.g. ${disagree.slice(0, 3).join("; ")}` : ""); }

console.log(`part_off_web_e2e: ${fails ? "FAIL" : "ok"} (${fails} failing cases)`);
process.exit(fails ? 1 : 0);
