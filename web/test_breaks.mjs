// SPDX-License-Identifier: GPL-3.0-only
// web/x0x_breaks.js against tools/upload_breaks.py: the same WAV files give the same slot images,
// byte for byte, whole and cut to one bar, at 44.1 kHz stereo 16-bit and 48 kHz mono 24-bit.
//   node web/test_breaks.mjs
import { execFileSync } from "node:child_process";
import { existsSync, mkdtempSync, readFileSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { readWav, cut, cutLoop, pack, userSlot, userSlotRaw, imaEncode, DRY_SLOT_KB, RATE, RATE_HALF, imaDecode, trimSilence, parseSlot, parseInfo, makeArchive, readArchive,
  planRestore } from "./x0x_breaks.js";

const dir = mkdtempSync(join(tmpdir(), "x0x-breaks-"));
let bad = 0;

function wav(path, rate, nch, bits, seconds, seed, pad = 0) {
  const n = Math.round(rate * seconds), bps = bits >> 3, np = Math.round(rate * pad);
  const data = Buffer.alloc((n + 2 * np) * nch * bps);
  let r = seed;
  for (let i = 0; i < n; i++) {
    r = (r * 1103515245 + 12345) >>> 0;
    const beat = (i % Math.round(rate / 2)) < rate * 0.03 ? 0.8 : 0.0;   // a click on every 8th at 120
    const v = beat * Math.sin(i * 0.3) + 0.1 * ((r >>> 8) / 16777216 - 0.5) + 0.2 * Math.sin(i * 2 * Math.PI * 110 / rate);
    for (let c = 0; c < nch; c++) {
      const s = Math.max(-1, Math.min(1, c ? v * 0.7 : v));
      const o = ((i + np) * nch + c) * bps;           // pad: silence before and after
      if (bits === 16) data.writeInt16LE(Math.round(s * 32767), o);
      else data.writeIntLE(Math.round(s * 8388607), o, 3);
    }
  }
  const h = Buffer.alloc(44);
  h.write("RIFF", 0); h.writeUInt32LE(36 + data.length, 4); h.write("WAVE", 8); h.write("fmt ", 12);
  h.writeUInt32LE(16, 16); h.writeUInt16LE(1, 20); h.writeUInt16LE(nch, 22); h.writeUInt32LE(rate, 24);
  h.writeUInt32LE(rate * nch * bps, 28); h.writeUInt16LE(nch * bps, 32); h.writeUInt16LE(bits, 34);
  h.write("data", 36); h.writeUInt32LE(data.length, 40);
  writeFileSync(path, Buffer.concat([h, data]));
}

const files = [
  [join(dir, "amen.wav"), 44100, 2, 16, 4.0, 1],     // 2 s per bar at 120: --bars cuts it to 2 s
  [join(dir, "think.wav"), 48000, 1, 24, 2.1, 7],
  [join(dir, "funky.wav"), 44100, 1, 16, 1.6, 3],
  [join(dir, "shot.wav"), 48000, 1, 16, 0.5, 5, 0.4], // 0.4 s of silence before and after: --trim cuts it
];
for (const f of files) wav(...f);

// the defaults (44.1 kHz, no trim, the SMK-37's 80 / 120 KB), 22.05 kHz on request, trim on request,
// other layouts (equal slots; the older 2 x 80 KB)
const D = DRY_SLOT_KB;
const cases = [
  { what: "whole, 44.1 kHz", bars: false, rate: RATE, trim: false, kb: D, args: [] },
  { what: "one bar, 44.1 kHz", bars: true, rate: RATE, trim: false, kb: D, args: ["--bars"] },
  { what: "whole, 22.05 kHz", bars: false, rate: RATE_HALF, trim: false, kb: D, args: ["--rate", "22050"] },
  { what: "one bar, trimmed", bars: true, rate: RATE, trim: true, kb: D, args: ["--bars", "--trim"] },
  { what: "one bar, oldschool", bars: true, rate: RATE, trim: false, kb: D, args: ["--bars", "--oldschool"], oldschool: true },
  { what: "shot, trimmed", bars: false, rate: RATE, trim: true, kb: D, args: ["--trim"], only: [3] },
  { what: "whole, 3 x 120 KB", bars: false, rate: RATE, trim: false, kb: [120, 120, 120], args: ["--nslots", "3", "--slot-kb", "120"] },
  { what: "whole, 2 x 80 KB", bars: false, rate: RATE, trim: false, kb: [80, 80], args: ["--slot-kb", "80,80"] },
];
for (const c of cases) {
  const out = join(dir, c.what.replace(/[^a-z0-9]/gi, "_"));
  const fs = c.only ? c.only.map((k) => files[k]) : files;
  execFileSync("python3", ["tools/upload_breaks.py", ...c.args, "--dry-run", out, ...fs.map((f) => f[0])], { stdio: "pipe" });
  const loops = fs.map((f) => {
    const { rate, x } = readWav(readFileSync(f[0]));
    if (c.oldschool) {
      const { s, rate: playback } = cutLoop(x, rate, c.bars, c.rate, c.trim, 2);
      return { label: f[0], s, playback };
    }
    return { label: f[0], s: cut(x, rate, c.bars, c.rate, c.trim), playback: c.rate };
  });
  const { slots } = pack(loops, c.kb.length, c.kb.map((k) => k * 1024 - 512));
  slots.forEach((sl, i) => {
    if (!sl.length) {                                  // an empty slot: upload_breaks.py writes no files for it
      const none = !existsSync(join(out, `slot${i + 1}.hdr`));
      console.log(`  ${none ? "ok" : "FAIL"} ${c.what}, slot ${i + 1}: empty`);
      if (!none) bad++;
      return;
    }
    const img = c.oldschool
      ? userSlotRaw(`BR${i + 1}`, sl.map((l) => ({ adpcm: imaEncode(l.s), n: l.s.length, rate: l.playback })), c.kb[i] * 1024)
      : userSlot(`BR${i + 1}`, sl.map((l) => l.s), c.rate, c.kb[i] * 1024);
    const hdr = readFileSync(join(out, `slot${i + 1}.hdr`)), bin = readFileSync(join(out, `slot${i + 1}.bin`));
    const same = Buffer.compare(Buffer.from(img.hdr), hdr) === 0 && Buffer.compare(Buffer.from(img.data), bin) === 0;
    console.log(`  ${same ? "ok" : "FAIL"} ${c.what}, slot ${i + 1}: ${sl.length} loop(s), ${img.data.length} B`);
    if (!same) bad++;
  });
}
// trim: the silence goes, 1 ms stays before the sound, the tail fades to 0; loops are not trimmed by default
{
  const { rate, x } = readWav(readFileSync(files[3][0]));
  const y = trimSilence(x, rate), first = y.findIndex((v) => Math.abs(v) > 0.001);
  const ok = y.length < x.length * 0.6 && first === 48 && y[y.length - 1] === 0 && cut(x, rate, false).length > cut(x, rate, false, RATE, true).length;
  console.log(`  ${ok ? "ok" : "FAIL"} trim: ${x.length} -> ${y.length} samples, sound at ${first} (1 ms), faded end`);
  if (!ok) bad++;
}
// reading back: the header parses, and the decoded loops follow what went in (ADPCM: close, not exact)
{
  const loops = files.map((f) => { const { rate, x } = readWav(readFileSync(f[0])); return cut(x, rate, true, RATE_HALF); });
  const img = userSlot("BR1", loops, RATE_HALF), h = parseSlot(img.hdr);
  let ok = h && h.nz === loops.length && h.name === "BR1" && h.dataLen === img.data.length && h.zones.every((z) => z.rate === RATE_HALF);
  loops.forEach((s, i) => {
    const z = h.zones[i], y = imaDecode(img.data.subarray(z.off, z.off + ((z.n + 1) >> 1)), z.n);
    let e = 0, p = 0;
    for (let k = 0; k < s.length; k++) { e += (y[k] - s[k]) ** 2; p += s[k] ** 2; }
    ok = ok && z.n === s.length && e / p < 0.05;
  });
  console.log(`  ${ok ? "ok" : "FAIL"} read back: header and decoded loops`);
  if (!ok) bad++;
}
// slot backup: the page's .x0xslots archive is tools/x0x_slots.py's, byte for byte, both ways
{
  const out = join(dir, "py.x0xslots");
  const info = JSON.parse(execFileSync("python3", ["-c", `
import json, sys
sys.path.insert(0, "tools")
import sampleio as sio, x0x_slots as xs
h, d = sio.user_slot("BR1", [([int(3000 * ((i * 7) % 13 - 6)) for i in range(30001)], 36, None, None),
                             ([int(2000 * ((i * 5) % 11 - 5)) for i in range(20000)], 37, None, None)], rate=44100)
img = h.ljust(512, bytes([255])) + d
info = [3, 120, 2, 66, 82, 49, 0, 49, 0, 0, 0, 0, 0, 0]
lay = xs.parse_info(info)
open(sys.argv[1], "wb").write(xs.make_archive(lay, [img, None, None], created="2026-10-15T12:00:00"))
ext = [3, 44, 2, 66, 82, 49, 0, 49, 0, 0, 0, 0, 0, 0, 88, 1]     # the SMK-37: 80 / 120 / 44 KiB, with the X0X extension
for off, size, used in ((0xC2000, 0x14000, 49), (0xD6000, 0x1E000, 0), (0xB0000, 0xB000, 0)):
    for v in (off >> 12, size >> 12, used):
        ext += [v & 0x7F, v >> 7]
open(sys.argv[1] + ".ext", "wb").write(xs.make_archive(xs.parse_info(ext), [img, None, None], created="2026-10-15T12:00:00"))
print(json.dumps({"info": info, "ext": ext, "img": list(img)}))
`, out]).toString());
  const py = new Uint8Array(readFileSync(out)), lay = parseInfo(info.info), img = new Uint8Array(info.img);
  const js = makeArchive(lay, [img, null, null], "2026-10-15T12:00:00");
  const same = Buffer.compare(Buffer.from(js), Buffer.from(py)) === 0;
  console.log(`  ${same ? "ok" : "FAIL"} backup archive: same bytes as x0x_slots.py (${js.length} B)`);
  if (!same) bad++;
  {
    const pe = new Uint8Array(readFileSync(out + ".ext")), le = parseInfo(info.ext);
    const je = makeArchive(le, [img, null, null], "2026-10-15T12:00:00"), m = readArchive(je).meta;
    const same = Buffer.compare(Buffer.from(je), Buffer.from(pe)) === 0 && le.ext && m.slots.map((e) => e.size).join() === "81920,122880,45056"
      && m.slots[2].flash === 0xB0000;
    console.log(`  ${same ? "ok" : "FAIL"} backup archive, SMP_INFO extension (80 / 120 / 44 KB): same bytes as x0x_slots.py`);
    if (!same) bad++;
    const pl = planRestore(le, [img, img, img]);
    const pok = pl.todo.length === 3 && planRestore(le, [null, null, new Uint8Array(50000)]).skipped.length === 1;
    console.log(`  ${pok ? "ok" : "FAIL"} restore plan: per-slot sizes (50000 B do not fit the 44 KB slot 3)`);
    if (!pok) bad++;
  }
  const { meta, images } = readArchive(py);
  const ok = lay.count === 3 && lay.size === 120 * 1024 && meta.slots[0].rates.join() === "44100,44100" &&
    meta.slots[0].name === "BR1" && images[0].length === img.length && images[1] === null;
  console.log(`  ${ok ? "ok" : "FAIL"} backup archive: read back (3 x 120 KB layout, rates, name)`);
  if (!ok) bad++;
  const dmg = py.slice();
  dmg[100] ^= 1;
  let refused = false;
  try { readArchive(dmg); } catch (_) { refused = true; }
  console.log(`  ${refused ? "ok" : "FAIL"} backup archive: a damaged file is refused`);
  if (!refused) bad++;
  const plan = planRestore({ count: 2, size: 80 * 1024, slots: [] }, images);
  const pok = plan.todo.length === 2 && plan.skipped.length === 1;
  console.log(`  ${pok ? "ok" : "FAIL"} restore plan: a 3-slot backup on a 2-slot device skips slot 3`);
  if (!pok) bad++;
}
console.log(bad ? "BREAKS WEB FAILED" : "breaks web: same bytes as upload_breaks.py");
process.exit(bad ? 1 : 0);
