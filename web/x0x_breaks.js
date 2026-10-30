import { watchDeviceSession } from "./device_session.js";
// SPDX-License-Identifier: GPL-3.0-only
// X0X break loops in the browser: tools/upload_breaks.py and its helpers (tools/sampleio.py,
// Felucca's encoder; tools/import_breaks.py's bar guess) as plain functions, so the page
// (web/x0x_breaks.html) builds the same slot images the command line does, byte for byte
// (web/test_breaks.mjs checks it against upload_breaks.py --dry-run).
//
// The float arithmetic follows sampleio.py's order of operations: keep it when touching these.

export const SLOT_SIZE = 0x14000, SLOT_DATA_OFF = 512, SLOT_ZONES = 16;
export const SLOT_HDR_LEN = 32 + SLOT_ZONES * 28;
export const SLOT_MAX_DATA = SLOT_SIZE - SLOT_DATA_OFF;
export const ROOT_BASE = 36;              // loop i is zone root 36 + i: the slot sorts its zones by root
export const RATE = 44100, RATE_HALF = 22050;   // 44.1 kHz by default; 22.05 kHz only when asked for
export const OLDSCHOOL_FACTOR = 2;              // --oldschool CLI default (medium grit)
export const MIN_LOOP_SAMPLES = 64;           // firmware breaks_set_loop minimum
/** @type {{ key: string, label: string, factor: number }[]} */
export const OLDSCHOOL_GRIT = [
  { key: "", label: "Off (clean)", factor: 1 },
  { key: "light", label: "Light", factor: 1.5 },
  { key: "medium", label: "Medium", factor: 2 },
  { key: "heavy", label: "Heavy", factor: 3 },
  { key: "max", label: "Max", factor: 4 },
];
export const DRY_SLOT_KB = [80, 120]; // upload_breaks.py --dry-run's default layout: the SMK-37's two slots (KiB)

const IMA_STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88,
  97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
  724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660,
  4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
  18500, 20350, 22385, 24623, 27086, 29794, 32767];
const IMA_IDX = [-1, -1, -1, -1, 2, 4, 6, 8];

// import_breaks.bars_of: the bar count (1, 2 or 4) that puts the loop's tempo nearest 120 inside 80-180
export function barsOf(seconds) {
  let best = null;
  for (const b of [1, 2, 4]) {
    const bpm = b * 240.0 / seconds;
    if (bpm >= 80 && bpm <= 180 && (best === null || Math.abs(bpm - 120) < Math.abs(best[1] - 120))) best = [b, bpm];
  }
  return best || [1, 240.0 / seconds];
}

// sampleio.resample: a moving average over the ratio, then linear interpolation
export function resample(x, sr, to) {
  if (sr === to) return x;
  if (sr > to) {
    const k = Math.max(1, Math.round(sr / to));
    if (k === 2) {
      const y = new Array(x.length);
      for (let i = 0; i + 1 < x.length; i++) y[i] = (x[i] + x[i + 1]) / 2;
      if (x.length) y[x.length - 1] = x[x.length - 1] / 2;
      x = y;
    } else if (k > 1) {
      const y = new Array(x.length);
      for (let i = 0; i < x.length; i++) {
        let acc = x[i];                           // reduce(add, x[i:i + k]): left to right
        for (let j = i + 1; j < i + k && j < x.length; j++) acc += x[j];
        y[i] = acc / k;
      }
      x = y;
    }
  }
  const step = sr / to, out = [], last = x.length - 1;
  let p = 0.0;
  while (p < last) {
    const i = Math.trunc(p), f = p - i;
    out.push(x[i] * (1 - f) + x[i + 1] * f);
    p += step;
  }
  return out;
}

function peak(x, floor = 1e-9) {
  let m = floor;
  for (const v of x) { const a = Math.abs(v); if (a > m) m = a; }
  return m;
}

// sampleio.to_int16: peak normalised to 30000
export function toInt16(x) {
  const pk = peak(x), out = new Int16Array(x.length);
  for (let i = 0; i < x.length; i++) out[i] = Math.max(-32768, Math.min(32767, Math.trunc(x[i] / pk * 30000)));
  return out;
}

// sampleio.trim_silence: cut the silence at both ends (floor 0.001 = -60 dBFS), keep 1 ms before the
// sound, keep 5 ms after it and fade that tail to 0
export function trimSilence(x, sr, floor = 0.001, preMs = 1, fadeMs = 5) {
  let first = -1, last = -1;
  for (let i = 0; i < x.length; i++) if (Math.abs(x[i]) > floor) { if (first < 0) first = i; last = i; }
  if (first < 0) return x;
  const pre = Math.floor(sr * preMs / 1000), nf = Math.floor(sr * fadeMs / 1000);
  const y = x.slice(Math.max(0, first - pre), Math.min(x.length, last + 1 + nf)), m = Math.min(nf, y.length);
  for (let j = 0; j < m; j++) { const k = y.length - m + j; y[k] = y[k] * ((m - 1 - j) / m); }
  return y;
}

// upload_breaks.cut: mono floats at their rate -> int16 at `rate` (one bar if bars; silence cut if trim)
export function cut(x, sr, bars, rate = RATE, trim = false) {
  if (trim) x = trimSilence(x, sr);
  if (bars) {
    const n = barsOf(x.length / sr)[0];
    x = x.slice(0, Math.round(x.length / n));
  }
  const y = resample(x, sr, rate);
  const pk = peak(y);
  return toInt16(y.map((v) => v * (0.95 / pk)));
}

// Optional "oldschool" import: speed the audio up (fewer samples), store a lower zone playback rate so
// the BREAK player stretches it back to the same bar length (grit / aliasing from the encode path stay).
export function applyOldschool(s, storageRate, factor = OLDSCHOOL_FACTOR) {
  if (factor <= 1) return { s, rate: storageRate };
  const x = [];
  for (let i = 0; i < s.length; i++) x.push(s[i] / 32768);
  const y = resample(x, storageRate, storageRate / factor);
  const out = toInt16(y.map((v) => v * 0.95));
  const playbackRate = Math.round(storageRate / factor);
  if (out.length < MIN_LOOP_SAMPLES)
    throw new Error(`too short after oldschool processing (${out.length} samples)`);
  return { s: out, rate: playbackRate };
}

export function cutLoop(x, sr, bars, storageRate = RATE, trim = false, gritFactor = 1) {
  let s = cut(x, sr, bars, storageRate, trim);
  let rate = storageRate;
  if (gritFactor > 1) ({ s, rate } = applyOldschool(s, storageRate, gritFactor));
  return { s, rate };
}

export function gritLabel(factor) {
  const g = OLDSCHOOL_GRIT.find((o) => o.factor === factor);
  return g && g.factor > 1 ? g.label : "";
}

// What you hear on the device: IMA round-trip at the zone playback rate (stretched timing).
export function loopPreviewPcm(s, playbackRate, roundTrip = true) {
  const pcm = roundTrip ? imaDecode(imaEncode(s), s.length) : s;
  const samples = new Float32Array(pcm.length);
  for (let i = 0; i < pcm.length; i++) samples[i] = pcm[i] / 32768;
  return { samples, rate: playbackRate };
}

// sampleio.ima_encode: 4-bit IMA ADPCM, low nibble first, from predictor 0 / index 0
export function imaEncode(s) {
  let pred = 0, idx = 0;
  const out = new Uint8Array((s.length + 1) >> 1);
  for (let n = 0; n < s.length; n++) {
    const step = IMA_STEP[idx];
    let diff = s[n] - pred, code = 0;
    if (diff < 0) { code = 8; diff = -diff; }
    let vd = step >> 3;
    if (diff >= step) { code |= 4; diff -= step; vd += step; }
    if (diff >= step >> 1) { code |= 2; diff -= step >> 1; vd += step >> 1; }
    if (diff >= step >> 2) { code |= 1; vd += step >> 2; }
    pred = code & 8 ? pred - vd : pred + vd;
    if (pred > 32767) pred = 32767; else if (pred < -32768) pred = -32768;
    idx += IMA_IDX[code & 7];
    if (idx < 0) idx = 0; else if (idx > 88) idx = 88;
    out[n >> 1] |= n & 1 ? code << 4 : code;
  }
  return out;
}

// the reverse, for drawing a slot read back from the FM-1: nibbles -> int16, from predictor 0 / index 0
export function imaDecode(b, n) {
  let pred = 0, idx = 0;
  const out = new Int16Array(n);
  for (let i = 0; i < n; i++) {
    const code = (b[i >> 1] >> (i & 1 ? 4 : 0)) & 15, step = IMA_STEP[idx];
    let vd = step >> 3;
    if (code & 4) vd += step;
    if (code & 2) vd += step >> 1;
    if (code & 1) vd += step >> 2;
    pred = code & 8 ? pred - vd : pred + vd;
    if (pred > 32767) pred = 32767; else if (pred < -32768) pred = -32768;
    idx += IMA_IDX[code & 7];
    if (idx < 0) idx = 0; else if (idx > 88) idx = 88;
    out[i] = pred;
  }
  return out;
}

// a slot header (userSlot's layout) -> {nz, name, dataLen, zones: [{off, n, rate}]}, or null if not a slot
export function parseSlot(hdr) {
  const v = new DataView(hdr.buffer || hdr, hdr.byteOffset || 0, hdr.byteLength);
  if (hdr.byteLength < SLOT_HDR_LEN || v.getUint32(0, true) !== 0x504D5346) return null;
  const nz = hdr[6], zones = [];
  let name = "";
  for (let i = 8; i < 16 && hdr[i]; i++) name += String.fromCharCode(hdr[i]);
  for (let i = 0; i < nz && i < SLOT_ZONES; i++)
    zones.push({ off: v.getUint32(32 + i * 28, true), n: v.getUint32(32 + i * 28 + 4, true),
      rate: Math.round(v.getUint32(32 + i * 28 + 16, true) * 44100 / 65536) });
  return { nz, name, dataLen: v.getUint32(16, true), zones };
}

let CRC_T = null;
export function crc32(b) {                    // zlib.crc32
  if (!CRC_T) {
    CRC_T = new Uint32Array(256);
    for (let n = 0; n < 256; n++) {
      let c = n;
      for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1;
      CRC_T[n] = c >>> 0;
    }
  }
  let c = 0xFFFFFFFF;
  for (let i = 0; i < b.length; i++) c = CRC_T[(c ^ b[i]) & 0xFF] ^ (c >>> 8);
  return (c ^ 0xFFFFFFFF) >>> 0;
}

function keySplit(roots) {
  const n = roots.length;
  return roots.map((r, j) => [j === 0 ? 0 : ((roots[j - 1] + r) >> 1) + 1, j === n - 1 ? 127 : (r + roots[j + 1]) >> 1]);
}

// sampleio.user_slot for loops at `rate` (each its own zone, roots ROOT_BASE + i): -> {hdr, data}
export function userSlot(name, loops, rate = RATE, slotSize = SLOT_SIZE) {
  return userSlotRaw(name, loops.map((s) => ({ adpcm: imaEncode(s), n: s.length, rate })), slotSize);
}

// the same from loops already encoded ({adpcm, n, rate}: the break page keeps a loop read back as it is,
// at its own rate)
export function userSlotRaw(name, parts, slotSize = SLOT_SIZE) {
  const total = parts.reduce((a, p) => a + p.adpcm.length, 0), max = slotSize - SLOT_DATA_OFF;
  if (total > max) throw new Error(`too long: ${total} B of ADPCM, a slot holds ${max} B`);
  if (parts.length < 1 || parts.length > SLOT_ZONES) throw new Error(`1..${SLOT_ZONES} loops per slot`);
  const data = new Uint8Array(total);
  const zs = [];
  let off = 0;
  parts.forEach((p, i) => {
    data.set(p.adpcm, off);
    zs.push({ off, n: p.n, root: ROOT_BASE + i, q16: Math.round(p.rate / 44100 * 65536) });
    off += p.adpcm.length;
  });
  const split = keySplit(zs.map((z) => z.root));
  const hdr = new Uint8Array(SLOT_HDR_LEN), v = new DataView(hdr.buffer);
  v.setUint32(0, 0x504D5346, true);
  v.setUint16(4, 1, true);
  hdr[6] = zs.length;
  const nm = name.toUpperCase().replace(/[^\x20-\x7E]/g, "").slice(0, 8);
  for (let i = 0; i < nm.length; i++) hdr[8 + i] = nm.charCodeAt(i);
  v.setUint32(16, data.length, true);
  v.setUint32(20, crc32(data), true);
  zs.forEach((z, i) => {
    const o = 32 + i * 28;
    v.setUint32(o, z.off, true);
    v.setUint32(o + 4, z.n, true);
    v.setUint32(o + 8, 0, true);               // loop start
    v.setUint32(o + 12, z.n - 1, true);        // loop end
    v.setUint32(o + 16, z.q16, true);
    v.setInt16(o + 20, z.root * 16, true);
    v.setInt16(o + 22, 0, true);               // predictor and index at the loop start: from 0
    hdr[o + 24] = 0;
    hdr[o + 25] = split[i][0];
    hdr[o + 26] = split[i][1];
  });
  return { hdr, data };
}

// upload_breaks.pack: loops [{label, s}] in order into at most nslots slots; cap: ADPCM bytes a slot, one
// number or one per slot -> {slots: one list per slot (empty ones included, none after the last used one),
// skipped}. A loop goes into the current slot if it fits, else into the next slot that can take it.
export function pack(loops, nslots, cap = SLOT_MAX_DATA) {
  const caps = Array.isArray(cap) ? cap : new Array(nslots).fill(cap);
  const slots = Array.from({ length: nslots }, () => []), skipped = [];
  let cur = 0, used = 0;
  for (const l of loops) {
    const need = (l.s.length + 1) >> 1;
    let j = cur;
    while (j < nslots && !((j === cur ? used : 0) + need <= caps[j] && slots[j].length < SLOT_ZONES)) j++;
    if (j >= nslots) { skipped.push(l.label); continue; }
    if (j !== cur) { cur = j; used = 0; }
    slots[cur].push(l);
    used += need;
  }
  while (slots.length && !slots[slots.length - 1].length) slots.pop();
  return { slots, skipped };
}

// fm1_sample_upload.pack7: groups of up to 7 bytes, each preceded by their top bits (7 to 8: the slot
// commands' packing; fm1ota.js's pack7 is the update protocol's bit stream, another thing)
export function pack78(b) {
  const out = [];
  for (let i = 0; i < b.length; i += 7) {
    const g = b.slice(i, i + 7);
    let m = 0;
    g.forEach((x, j) => { m |= ((x >> 7) & 1) << j; });
    out.push(m);
    for (const x of g) out.push(x & 0x7F);
  }
  return out;
}

// a 16-bit / 24-bit / float WAV -> {rate, x: mono floats} (sampleio.read_any_wav; the page decodes
// other formats with the browser's decoder)
export function readWav(buf) {
  const d = new DataView(buf.buffer || buf, buf.byteOffset || 0, buf.byteLength);
  const tag4 = (o) => String.fromCharCode(d.getUint8(o), d.getUint8(o + 1), d.getUint8(o + 2), d.getUint8(o + 3));
  if (d.byteLength < 12 || tag4(0) !== "RIFF" || tag4(8) !== "WAVE") throw new Error("not a WAV file");
  const ch = {};
  for (let i = 12; i + 8 <= d.byteLength;) {
    const id = tag4(i), n = d.getUint32(i + 4, true);
    if (!(id in ch)) ch[id] = [i + 8, n];
    i += 8 + n + (n & 1);
  }
  if (!ch["fmt "]) throw new Error("WAV without a fmt chunk");
  const fo = ch["fmt "][0];
  let tag = d.getUint16(fo, true);
  const nch = d.getUint16(fo + 2, true), sr = d.getUint32(fo + 4, true), bits = d.getUint16(fo + 14, true);
  if (tag === 0xFFFE) tag = d.getUint16(fo + 24, true);
  const bps = bits >> 3;
  if ((tag !== 1 && tag !== 3) || !bps || !nch) throw new Error(`unsupported WAV (tag ${tag}, ${bits} bit)`);
  const [o, n] = ch.data || [0, 0];
  const frame = bps * nch, nfr = Math.floor(Math.min(n, d.byteLength - o) / frame);
  const val = (p) => {
    if (tag === 3) return bps === 4 ? d.getFloat32(p, true) : d.getFloat64(p, true);
    if (bps === 2) return d.getInt16(p, true) / 32768;
    if (bps === 3) return ((d.getUint8(p) << 8 | d.getUint8(p + 1) << 16 | d.getUint8(p + 2) << 24) >> 0) / 2147483648;
    if (bps === 4) return d.getInt32(p, true) / 2147483648;
    return (d.getUint8(p) - 128) / 128;
  };
  const x = new Array(nfr);
  for (let f = 0; f < nfr; f++) {
    const p = o + f * frame;
    if (nch === 1) x[f] = val(p);
    else if (nch === 2) x[f] = (val(p) + val(p + bps)) / 2;
    else { let acc = 0.0; for (let c = 0; c < nch; c++) acc += val(p + c * bps); x[f] = acc / nch; }
  }
  return { rate: sr, x };
}

// ---- slot backup and restore: tools/x0x_slots.py's .x0xslots archive, byte for byte ----------
// "X0XSLOTS", u16 version 1, u16 0, u32 L, L bytes of JSON, the slot images, u32 CRC-32 of all before.
// An image is a slot from its start: the 512-byte header area, then the ADPCM data.

export const SLOT_HDR_AREA = 512;
const ARCH_MAGIC = "X0XSLOTS";

// a slot name as text: printable ASCII kept, anything else "?" (x0x_slots.ascii_name)
export function asciiName(b) {
  let s = "";
  for (const c of b) s += c >= 32 && c < 127 ? String.fromCharCode(c) : "?";
  return s;
}

// SMP_INFO reply (the bytes after the command) -> {count, size, ext, slots: [{nz, name, kb, size, off, used}]}
// (x0x_slots.parse_info). size: the reply's one size (KiB, 7 bits: the smallest slot). Per slot size, flash
// offset (null if unknown) and used KiB come from X0X's extension ('X', 1, then per slot offset and size in
// 4 KiB sectors and used KiB, 14 bits each); without it every slot has the one size.
export function parseInfo(r) {
  if (!r || r.length < 2) throw new Error("short SMP_INFO reply");
  const count = r[0], size = r[1] * 1024, slots = [];
  if (count < 1 || count > 16 || !size || r[1] > 127)
    throw new Error("invalid SMP_INFO slot count or size");
  let i = 2;
  for (let k = 0; k < count; k++) {
    if (i >= r.length || r[i] > 16) throw new Error("invalid SMP_INFO slot record");
    const nz = r[i], j = r.indexOf(0, i + 1);
    // Device names have at most eight bytes and must end before used-KiB.
    if (j < 0 || j > i + 9 || j + 1 >= r.length || r[j + 1] > 127)
      throw new Error("truncated SMP_INFO slot record");
    // Empty slots must have empty names and zero used bytes. Otherwise a
    // damaged SMP_INFO descriptor could skip the required pre-OTA sample backup.
    if (nz === 0 && (j !== i + 1 || r[j + 1] !== 0))
      throw new Error(`slot ${k + 1}: empty SMP_INFO slot has occupied metadata`);
    slots.push({ nz, name: asciiName(r.slice(i + 1, j)), kb: r[j + 1],
      size, off: null, used: r[j + 1] });
    i = j + 2;
  }
  // Valid legacy X0X layouts can omit the extension altogether. If any
  // extension bytes exist, they MUST describe all declared slots completely.
  const ext = i < r.length;
  if (ext) {
    if (r[i] !== 0x58 || r[i + 1] !== 1 || r.length !== i + 2 + 6 * count)
      throw new Error("malformed or truncated SMP_INFO extension");
    slots.forEach((sl, k) => {
      const v = [0, 1, 2].map((m) => r[i + 2 + 6 * k + 2 * m] | r[i + 3 + 6 * k + 2 * m] << 7);
      if (!v[1] || v[2] > v[1] * 4)
        throw new Error("invalid SMP_INFO extended slot size");
      // Firmware saturates only the legacy byte; both fields describe the same used KiB.
      if (sl.kb !== Math.min(127, v[2]))
        throw new Error(`slot ${k + 1}: legacy/extended used-size disagreement`);
      Object.assign(sl, { off: v[0] * 0x1000, size: v[1] * 0x1000, used: v[2] });
    });
  }
  return { count, size, ext, slots };
}

// a slot's first 480 bytes -> {nz, name, dataLen, crc, zones: [{off, n, rate}]} or null (x0x_slots.parse_header)
export function parseHeader(b) {
  if (b.length < SLOT_HDR_LEN) return null;
  const v = new DataView(b.buffer, b.byteOffset, b.byteLength);
  const nz = b[6];
  if (v.getUint32(0, true) !== 0x504D5346 || v.getUint16(4, true) !== 1 || nz < 1 || nz > 16) return null;
  let e = 8;
  while (e < 16 && b[e]) e++;
  const zones = [];
  for (let z = 0; z < nz; z++) {
    const o = 32 + z * 28, q16 = v.getUint32(o + 16, true);
    zones.push({ off: v.getUint32(o, true), n: v.getUint32(o + 4, true), rate: Number((BigInt(q16) * 44100n + 32768n) >> 16n) });
  }
  return { nz, name: asciiName(b.subarray(8, e)), dataLen: v.getUint32(16, true), crc: v.getUint32(20, true), zones };
}

// layout: parseInfo(); images: per slot, Uint8Array or null -> the archive (x0x_slots.make_archive)
export function makeArchive(layout, images, created) {
  if (!layout || !Array.isArray(images) || images.length !== layout.count || layout.slots.length !== layout.count)
    throw new Error("The sample backup is incomplete.");
  const entries = [], blobs = [];
  let off = 0;
  images.forEach((img, k) => {
    const sl = layout.slots[k] || {};
    const e = { slot: k + 1, size: sl.size ?? layout.size, empty: img === null };
    // Source occupancy is authoritative: a CRC-valid archive marked empty
    // must never conceal a slot that SMP_INFO reported as occupied.
    if (sl.nz > 0 && img === null)
      throw new Error(`slot ${k + 1}: occupied source slot is missing from the backup`);
    if (sl.off !== null && sl.off !== undefined) e.flash = sl.off;
    if (img !== null) {
      const h = parseHeader(img);
      Object.assign(e, { name: h.name, zones: h.nz, samples: h.zones.map((z) => z.n), rates: h.zones.map((z) => z.rate),
        data_len: h.dataLen, data_crc32: h.crc, offset: off, length: img.length, crc32: crc32(img) });
      blobs.push(img);
      off += img.length;
    }
    entries.push(e);
  });
  const meta = { format: "x0xslots", version: 1, created, count: layout.count, size: layout.size, slots: entries };
  const js = new TextEncoder().encode(JSON.stringify(meta));
  const out = new Uint8Array(16 + js.length + off + 4), v = new DataView(out.buffer);
  for (let i = 0; i < 8; i++) out[i] = ARCH_MAGIC.charCodeAt(i);
  v.setUint16(8, 1, true);
  v.setUint16(10, 0, true);
  v.setUint32(12, js.length, true);
  out.set(js, 16);
  let p = 16 + js.length;
  for (const b of blobs) { out.set(b, p); p += b.length; }
  v.setUint32(p, crc32(out.subarray(0, p)), true);
  return out;
}

// archive bytes -> {meta, images}; throws on any damage (x0x_slots.read_archive)
export function readArchive(b) {
  const bad = (m) => { throw new Error(m); };
  const int = (n, lo, hi) => Number.isSafeInteger(n) && n >= lo && n <= hi;
  if (!(b instanceof Uint8Array) || b.length < 20 || b.length > 4 * 1024 * 1024 ||
      String.fromCharCode(...b.subarray(0, 8)) !== ARCH_MAGIC) bad("This is not an .x0xslots backup.");
  const v = new DataView(b.buffer, b.byteOffset, b.byteLength);
  if (v.getUint32(b.length - 4, true) !== crc32(b.subarray(0, b.length - 4))) bad("The backup file is damaged (CRC).");
  if (v.getUint16(8, true) !== 1 || v.getUint16(10, true) !== 0) bad("This backup has an unsupported format.");
  const n = v.getUint32(12, true), base = 16 + n;
  if (n > 65536 || base > b.length - 4) bad("The backup metadata exceeds the file.");
  const meta = JSON.parse(new TextDecoder("utf-8", { fatal: true }).decode(b.subarray(16, base)));
  if (!meta || meta.format !== "x0xslots" || meta.version !== 1 || !int(meta.count, 1, 16) ||
      !Array.isArray(meta.slots) || meta.slots.length !== meta.count || !int(meta.size, 512, 4 * 1024 * 1024))
    bad("The backup slot count or format is invalid.");
  const images = [];
  let used = 0;
  for (let k = 0; k < meta.count; k++) {
    const e = meta.slots[k];
    if (!e || e.slot !== k + 1 || typeof e.empty !== "boolean" || !int(e.size, 512, 4 * 1024 * 1024))
      bad(`Slot ${k + 1} is missing, reordered or invalid.`);
    if (e.empty) {
      if (["offset", "length", "crc32"].some((key) => Object.hasOwn(e, key))) bad(`Slot ${e.slot}: empty slot has image pointers.`);
      images.push(null); continue;
    }
    if (e.offset !== used || !int(e.length, 512, Math.min(e.size, b.length - 4 - base - used)) || !int(e.crc32, 0, 0xFFFFFFFF))
      bad(`Slot ${e.slot}: overlapping, missing or invalid image range.`);
    const img = b.slice(base + used, base + used + e.length), h = parseHeader(img);
    if (crc32(img) !== e.crc32 || !h || img.length !== SLOT_HDR_AREA + h.dataLen || crc32(img.subarray(SLOT_HDR_AREA)) !== h.crc)
      bad(`Slot ${e.slot} in the backup is damaged or invalid.`);
    const hv = new DataView(img.buffer);
    for (let z = 0; z < h.nz; z++) {
      const o = 32 + z * 28, rate = hv.getUint32(o + 16, true), zone = h.zones[z];
      if (zone.off + Math.ceil(zone.n / 2) > h.dataLen || img[o + 24] > 88 || !rate || rate > 4 * 65536)
        bad(`Slot ${e.slot}: invalid sample zone ${z + 1}.`);
    }
    images.push(img); used += img.length;
  }
  if (base + used !== b.length - 4) bad("The backup contains unreferenced sample data.");
  return { meta, images };
}

// a slot's size on a device with this layout (per slot with X0X's SMP_INFO extension)
export const slotSize = (layout, k) => (layout.slots[k] && layout.slots[k].size) || layout.size;

// which archive slots go where on a device with this layout -> {todo: [{k, img}], skipped: [text]}
export function planRestore(layout, images) {
  const todo = [], skipped = [];
  images.forEach((img, k) => {
    if (k >= layout.count) skipped.push(`slot ${k + 1}: this device has ${layout.count} slots`);
    else if (img !== null && img.length > slotSize(layout, k)) skipped.push(`slot ${k + 1}: too large for this device's slot ${k + 1}`);
    else todo.push({ k, img });
  });
  return { todo, skipped };
}

// ---- an install that changes the slot map (tools/x0x_slots.py and tools/fm1_install.py, the same rules) ----
// A slot keeps its data over an install when the new firmware has a slot at the same flash offset that is
// large enough: no install writes the slots. The others are saved first and written back after.
export const LEGACY_BASE = { smk: 0xC2000, fm1: 0xA0000 };   // X0X before the SMP_INFO extension: equal slots
const SLOTMAP_MAGIC = "X0X-SLOTMAP-1\0\0\0", LOADER_MARK = "FELUCCA-LOADER-1";

function findBytes(b, text) {
  const t = [...text].map((c) => c.charCodeAt(0));
  outer: for (let i = b.indexOf(t[0]); i >= 0 && i + t.length <= b.length; i = b.indexOf(t[0], i + 1)) {
    for (let j = 1; j < t.length; j++) if (b[i + j] !== t[j]) continue outer;
    return i;
  }
  return -1;
}

// [{off, size}] per slot of a parseInfo() layout (x0x_slots.flash_map)
export function flashMap(layout, board = "smk") {
  if (layout.ext) return layout.slots.map((sl) => ({ off: sl.off, size: sl.size }));
  return layout.slots.map((_, k) => ({ off: LEGACY_BASE[board] + k * layout.size, size: layout.size }));
}

// the slot map of a package's logical image (fm1pkg_make.slotmap_of): its slot map record; for an older X0X
// (Felucca loader, no record) the equal 80 KiB slots of that time; [] for any other package (stock firmware)
export function packageSlotMap(image, board = "smk") {
  const i = findBytes(image, SLOTMAP_MAGIC);
  if (i >= 0 && i + 24 <= image.length) {
    const v = new DataView(image.buffer, image.byteOffset, image.byteLength), n = v.getUint32(i + 16, true);
    const end = i + 20 + 8 * n;
    if (n <= 16 && end + 4 <= image.length && v.getUint32(end, true) === crc32(image.subarray(i, end))) {
      const m = [];
      for (let k = 0; k < n; k++) m.push({ off: v.getUint32(i + 20 + 8 * k, true), size: v.getUint32(i + 24 + 8 * k, true) });
      return m;
    }
  }
  if (findBytes(image, LOADER_MARK) < 0) return [];
  return Array.from({ length: board === "smk" ? 2 : 3 }, (_, k) => ({ off: LEGACY_BASE[board] + k * 0x14000, size: 0x14000 }));
}

// slot indexes that hold samples and that the new map does not keep (x0x_slots.at_risk)
export function atRisk(layout, oldMap, newMap) {
  const risk = [];
  layout.slots.forEach((sl, k) => {
    if (!sl.nz) return;
    const off = oldMap[k].off, need = SLOT_HDR_AREA + sl.used * 1024;
    if (!newMap.some((m) => m.off === off && m.size >= need)) risk.push(k);
  });
  return risk;
}

// -> {todo: [{k: old slot, j: new slot}], skipped: [text]} (x0x_slots.plan_migration): a slot at risk goes back
// to the slot with its number if it fits there, else to the first other slot that is empty, not kept and large enough
export function planMigration(images, risk, oldMap, newLayout, newMap) {
  const kept = new Set();
  newMap.forEach((m, j) => oldMap.forEach((o, k) => { if (images[k] !== null && !risk.includes(k) && o.off === m.off) kept.add(j); }));
  const taken = new Set(kept), todo = [], skipped = [];
  for (const k of risk) {
    const img = images[k], order = (k < newMap.length ? [k] : []).concat(newMap.map((_, j) => j).filter((j) => j !== k));
    const j = order.find((j) => !taken.has(j) && img.length <= newMap[j].size && !newLayout.slots[j].nz);
    if (j === undefined) skipped.push(`slot ${k + 1} (${img.length} B): no free slot of that size on the new firmware`);
    else { todo.push({ k, j }); taken.add(j); }
  }
  return { todo, skipped };
}

export const X0X_NBREAKS_BUILTIN = 2;
const LOOP_MENU_GUARD = "LOOP menu order or indices would change; saved break pattern A/B indices could point at different loops. "
  + "The backup is retained. Restore samples manually to a compatible layout, then check and re-assign LOOP A/B in every saved pattern before playback.";

function userLoopMenuIds(layout, images, origins = null) {
  // scan_loops lists each zone, not each slot. Preserve original slot identity:
  // identical sample bytes in distinct slots are not interchangeable indices.
  const ids = [];
  layout.slots.forEach((sl, k) => {
    for (let z = 0; z < sl.nz; z++)
      ids.push(origins === null ? `old:${k}:${z}` : origins[k] === null ? `new:${k}:${z}` : `old:${origins[k]}:${z}`);
  });
  return ids;
}

function loopMenuAfterMigration(oldLayout, images, risk, oldMap, newMap, newLayout, todo) {
  const imgs = newMap.map(() => null), origins = newMap.map(() => null);
  const slots = newLayout.slots.map((sl) => ({ ...sl }));
  images.forEach((img, k) => {
    if (risk.includes(k) || !img) return;
    const off = oldMap[k].off;
    const j = newMap.findIndex((m) => m.off === off);
    if (j < 0) return;
    imgs[j] = img; origins[j] = k;
    slots[j] = { ...oldLayout.slots[k] };
  });
  todo.forEach(({ k, j }) => { imgs[j] = images[k]; origins[j] = k; slots[j] = { ...oldLayout.slots[k] }; });
  return userLoopMenuIds({ ...newLayout, slots }, imgs, origins);
}

export function migrationReordersLoopMenu(oldLayout, images, risk, oldMap, newLayout, newMap, todo) {
  if (!todo.length) return false;
  const before = userLoopMenuIds(oldLayout, images);
  const after = loopMenuAfterMigration(oldLayout, images, risk, oldMap, newMap, newLayout, todo);
  // Compare absolute common positions; skipped middle slots can shift indices
  // even when surviving entries retain their relative order. Trailing-only
  // omissions or additions leave all remaining original positions unchanged.
  return before.slice(0, after.length).some((id, i) => id !== after[i]);
}

export function guardMigrationLoopMenu(oldLayout, images, risk, oldMap, newLayout, newMap, todo, skipped) {
  if (!migrationReordersLoopMenu(oldLayout, images, risk, oldMap, newLayout, newMap, todo)) return { todo, skipped };
  const extra = [...skipped];
  for (const { k } of todo) extra.push(`slot ${k + 1}: not restored automatically (${LOOP_MENU_GUARD})`);
  return { todo: [], skipped: extra };
}

// ---- the slot commands over a link with send / read / drain (fm1ota.js's Link): SysEx F0 7D 46 4C cmd .. F7 ----
const ED = [0xF0, 0x7D, 0x46, 0x4C];

export function unpack78(a) {
  const out = [];
  for (let i = 0; i < a.length;) {
    const m = a[i++];
    for (let j = 0; j < 7 && i < a.length; j++) out.push(a[i++] | ((m >> j) & 1) << 7);
  }
  return new Uint8Array(out);
}

// one command -> the reply's bytes after the command; a lost frame is sent again (every slot command is safe to repeat)
export async function slotReq(link, cmd, args, ms = 3000, tries = 3) {
  for (let t = 0; t < tries; t++) {
    link.drain();
    if (!link.send([...ED, cmd, ...args, 0xF7])) throw new Error("The USB connection was lost.");
    const end = Date.now() + ms;
    while (Date.now() < end) {
      const p = await link.read(end - Date.now());
      if (!p) break;
      if (p.length > 5 && p[0] === 0xF0 && p.at(-1) === 0xF7 && p[1] === ED[1] && p[2] === ED[2] && p[3] === ED[3] && p[4] === cmd) {
        const r = p.slice(5, -1);
        if (r.some((v) => !Number.isInteger(v) || v < 0 || v > 127)) throw new Error("Malformed sample reply.");
        if ([11, 13, 14].includes(cmd) && (r.length !== 2 || r[0] !== args[0])) continue;
        if (cmd === 12 && (r.length !== 5 || !args.slice(0, 4).every((v, i) => v === r[i]))) continue;
        if (cmd === 16 && (r.length < 5 || !args.slice(0, 4).every((v, i) => v === r[i]))) continue;
        return r;
      }
    }
  }
  throw new Error(`The keyboard did not answer (slot command ${cmd}).`);
}

export async function readSlotLayout(link, ms = 3000, tries = 3) { return parseInfo(await slotReq(link, 15, [], ms, tries)); }

async function readSlotBytes(link, k, off, n, onPart) {
  const out = new Uint8Array(n);
  for (let at = 0; at < n;) {
    const want = Math.min(224, n - at), adr = off + at;
    const packed = (await slotReq(link, 16, [k, adr & 0x7F, (adr >> 7) & 0x7F, (adr >> 14) & 0x7F, (want + 1) >> 1])).slice(4);
    const b = unpack78(packed);
    if (b.length < want || b.length > want + (want % 2) ||
        !pack78(b).every((v, i) => v === packed[i]) || pack78(b).length !== packed.length)
      throw new Error(`slot ${k + 1}: invalid sample read length or encoding`);
    if (!b.length) throw new Error(`slot ${k + 1}: no data`);
    out.set(b.subarray(0, Math.min(b.length, n - at)), at);
    at += Math.min(b.length, n - at);
    if (onPart) onPart(at, n);
  }
  return out;
}

// slot k as an image (the 512-byte header area, then the data), or null if it is empty. Reads only.
export async function readSlotImage(link, k, nz, onPart, slotSize = null) {
  if (!nz) return null;
  const area = await readSlotBytes(link, k, 0, SLOT_HDR_AREA), h = parseHeader(area.subarray(0, SLOT_HDR_LEN));
  if (!h || h.nz !== nz)
    throw new Error(`slot ${k + 1}: occupied sample header is unreadable, unsupported or mismatches SMP_INFO`);
  if (h.dataLen === 0 || (slotSize !== null && h.dataLen > slotSize - SLOT_HDR_AREA))
    throw new Error(`slot ${k + 1}: occupied sample data length exceeds the slot or is empty`);
  const data = await readSlotBytes(link, k, SLOT_HDR_AREA, h.dataLen, onPart);
  if (crc32(data) !== h.crc) throw new Error(`slot ${k + 1}: the data read back does not match its CRC`);
  const img = new Uint8Array(SLOT_HDR_AREA + data.length);
  img.set(area);
  img.set(data, SLOT_HDR_AREA);
  return img;
}

// every slot -> {layout, images} (x0x_slots.backup): SMP_INFO and SMP_READ only
export async function backupSlots(link, onPart) {
  const layout = await readSlotLayout(link), images = [];
  for (let k = 0; k < layout.count; k++)
    images.push(await readSlotImage(link, k, layout.slots[k].nz,
      onPart && ((d, n) => onPart(k, d, n)), layout.slots[k].size));
  return { layout, images };
}

// erase slot k and write the image (null: erase only); throws on a device error (x0x_slots.write_slot)
export async function writeSlotImage(link, k, img, onPart) {
  if (!img) {
    if ((await slotReq(link, 14, [k], 30000))[1]) throw new Error(`slot ${k + 1}: erase failed`);
    return;
  }
  if ((await slotReq(link, 11, [k], 10000))[1]) throw new Error(`slot ${k + 1}: begin (erase) failed`);
  const data = img.subarray(SLOT_HDR_AREA);
  for (let off = 0; off < data.length; off += 256) {
    const adr = SLOT_HDR_AREA + off;
    const r = await slotReq(link, 12, [k, adr & 0x7F, (adr >> 7) & 0x7F, (adr >> 14) & 0x7F, ...pack78(data.subarray(off, off + 256))]);
    if (r[4]) throw new Error(`slot ${k + 1}: write at 0x${adr.toString(16)} failed (${r[4]})`);
    if (onPart) onPart(Math.min(off + 256, data.length), data.length);
  }
  const e = await slotReq(link, 13, [k, ...pack78(img.subarray(0, SLOT_HDR_LEN))], 5000);
  if (e[1]) throw new Error(`slot ${k + 1}: the keyboard refused the header (${e[1]})`);
}

// write images[k] into new slot j for each {k, j}, then read each back and compare (x0x_slots.restore_mapped)
export async function restoreMapped(link, todo, images, onPart) {
  for (const { k, j } of todo) await writeSlotImage(link, j, images[k], onPart && ((d, n) => onPart(j, d, n)));
  const after = await readSlotLayout(link);
  for (const { k, j } of todo) {
    const got = await readSlotImage(link, j, after.slots[j].nz, undefined, after.slots[j].size);
    if (!got || got.length !== images[k].length || got.some((v, i) => v !== images[k][i]))
      throw new Error(`slot ${j + 1}: read back differs from the backup of slot ${k + 1}`);
  }
}

// an archive file name for now: x0x-slots-before-install-YYYYMMDD-HHMMSS.x0xslots, and its "created" text
export function archiveStamp(t = new Date()) {
  const p = (v) => String(v).padStart(2, "0");
  const created = `${t.getFullYear()}-${p(t.getMonth() + 1)}-${p(t.getDate())}T${p(t.getHours())}:${p(t.getMinutes())}:${p(t.getSeconds())}`;
  return { created, name: `x0x-slots-before-install-${created.replace(/[-:]/g, "").replace("T", "-")}.x0xslots` };
}

// The install with the slots kept (fm1_install.py's flow, for the installer page). up: fm1ota.js's Updater;
// hooks: {step(k, a) for up.install, say(key, detail), save(bytes, name) (the download), progress(f),
// sameKeyboard(model, packageModel) (fm1ota.js's: the slots are read only when the install can go ahead)}.
// 1. If a slot holds samples the new firmware does not keep: read every slot into an .x0xslots archive and
//    save it. 2. Install. 3. Reconnect to the new firmware, write the slots back into its map, read them back.
// Throws the install's errors as they are; a failure in step 3 throws code "slots" (the archive is saved).
// -> {product, move: null or {risk, todo, skipped, name}}
export async function installKeepingSlots(up, image, product, board, hooks) {
  const h = Object.assign({ step() {}, say() {}, save() {}, progress() {}, sameKeyboard: () => true }, hooks);
  const fail = (code, msg, detail = "") => Object.assign(new Error(msg), { code, detail });
  let move = null, checkedLayout = null, checkedSnapshot = null;
  const dev = await up.find((id) => !/^ota-/i.test(id.model));
  const session = dev && up.access && dev.link.input && dev.link.output
    ? watchDeviceSession(up.access, dev.link.input, dev.link.output, () => {}) : null;
  try {
  // Require a freshly detected device and an informed physical-model decision for published DEV.
  if (!dev && h.confirmModel) throw fail("notfound", "Verify the keyboard model before DEV installation.");
  if (dev) {
    try {
      if (!h.sameKeyboard(dev.id.model, product.split("_")[0])) throw fail("model", "Device family mismatch.");
      if (h.confirmModel && !h.confirmModel(dev.id, product)) throw fail("modelcancel", "Physical model not confirmed; nothing written.");
      let layout = null;
      try { layout = await readSlotLayout(dev.link, 1000, 2); }
      catch (e) {
        // Only the independently exercised stock Pro _016 no-response path
        // lacks SMP_INFO. Corrupt replies, USB failures, current X0X and
        // unclassified firmware must NEVER bypass the prewrite backup gate.
        const knownStockNoReply = dev.id && dev.id.text === "SMK-37 Pro_016" &&
          e && e.message === "The keyboard did not answer (slot command 15).";
        if (!knownStockNoReply)
          throw fail("slotlayout", "Cannot verify the current sample layout before install: " + (e && e.message || e));
      }
      checkedLayout = layout;
      if (layout && layout.slots.some((sl) => sl.nz)) {
        const oldMap = flashMap(layout, board), newMap = packageSlotMap(image, board);
        const risk = atRisk(layout, oldMap, newMap);
        if (risk.length) {
          h.say("slotsave", risk.map((k) => k + 1).join(", "));
          let saved;
          try {
            saved = await backupSlots(dev.link, (k, d, n) => h.progress((k + d / n) / layout.count));
          } catch (e) { throw fail("slotbackup", e.message); }
          checkedSnapshot = saved;
          const st = archiveStamp(), file = makeArchive(saved.layout, saved.images, st.created);
          readArchive(file);                                  // what was saved reads back
          // Do not start destructive OTA while an asynchronous backup export is
          // pending or after the owner has rejected/failed that export.
          // A legacy synchronous callback returning undefined is preserved;
          // the browser's current download-click fallback is NOT durability proof.
          try {
            const savedOut = await h.save(file, st.name);
            if (savedOut === false) throw new Error("Backup export was not confirmed.");
          } catch (e) {
            throw fail("slotbackup", "Backup export failed: " + (e && e.message || e), st.name);
          }
          // SMP_INFO can change while the backup is read. Use the exact verified
          // snapshot for preservation, just as the Python SlotMove.backup does.
          const savedMap = flashMap(saved.layout, board);
          move = { risk: atRisk(saved.layout, savedMap, newMap), oldMap: savedMap, newMap,
            layout: saved.layout, images: saved.images, name: st.name, todo: [], skipped: [] };
        }
      }
    } finally { dev.link.close(); }
  }
  // The owner may change or revoke an approved package during backup readback.
  // Repeat the selection check immediately before beginning destructive OTA.
  if (typeof h.beforeFirmwareWrite === "function" && !h.beforeFirmwareWrite())
    throw fail("selectionstale", "Firmware selection changed before writing; nothing was installed.");
  const ran = await up.install(image, product, h.step, { beforeUpgrade: async (fresh) => {
    // Names and model strings are shared across keyboards. Bind the fresh lookup
    // to the exact preflight port objects, and re-read the saved bytes after the
    // owner has reselected the durable backup. Refuse a changed target/snapshot.
    session?.assert();
    if (!dev || fresh.name !== dev.name || fresh.id.text !== dev.id.text ||
        fresh.link.input !== dev.link.input || fresh.link.output !== dev.link.output)
      throw fail("selectionstale", "The keyboard changed after backup. Reconnect and prepare again.");
    if (checkedLayout) {
      const current = checkedSnapshot ? await backupSlots(fresh.link) : { layout: await readSlotLayout(fresh.link) };
      const expected = checkedSnapshot || { layout: checkedLayout };
      if (JSON.stringify(current.layout) !== JSON.stringify(expected.layout) || checkedSnapshot &&
          current.images.some((img, k) => (img === null) !== (expected.images[k] === null) || img &&
            (img.length !== expected.images[k].length || img.some((v, i) => v !== expected.images[k][i]))))
        throw fail("slotbackup", "Samples changed after backup. Nothing was installed; make a fresh backup.");
    }
    if (typeof h.beforeFirmwareWrite === "function" && !h.beforeFirmwareWrite())
      throw fail("selectionstale", "Firmware selection changed before writing; nothing was installed.");
    session?.assert();
    session?.close(); // The intentional OTA transition starts only after this guard.
  } });
  if (!move || !move.newMap.length) return { product: ran, move };
  h.say("slotback");
  // Same fail-closed rule as the post-write wait: reopen the port install just accepted, not another keyboard.
  const back = await up.reconnectAfter((id) => !/^ota-/i.test(id.model), 20000);
  if (!back) throw fail("slots", "the keyboard did not answer after the install", move.name);
  try {
    const layout = await readSlotLayout(back.link);
    let plan = planMigration(move.images, move.risk, move.oldMap, layout, flashMap(layout, board));
    plan = guardMigrationLoopMenu(move.layout, move.images, move.risk, move.oldMap, layout,
      flashMap(layout, board), plan.todo, plan.skipped);
    move.todo = plan.todo;
    move.skipped = plan.skipped;
    const total = plan.todo.length || 1;
    await restoreMapped(back.link, plan.todo, move.images, (j, d, n) => h.progress(d / n / total));
  } catch (e) {
    throw fail("slots", e.message, move.name);
  } finally { back.link.close(); }
  return { product: ran, move };
  } finally { session?.close(); }
}

// A break pack: whole IMA ADPCM of each loop, from the first sample. tools/break_pack.py writes the same bytes.
function packCrc32(bytes) {
  let crc = 0xFFFFFFFF;
  for (let i = 0; i < bytes.length; i++) {
    crc ^= bytes[i];
    for (let b = 0; b < 8; b++) crc = (crc >>> 1) ^ (0xEDB88320 & -(crc & 1));
  }
  return (~crc) >>> 0;
}

export function encodeBreakPack(loops, rate = RATE) {
  if (!loops.length || loops.length > 8) throw new Error("a pack holds 1 to 8 loops");
  if (rate !== RATE && rate !== RATE_HALF) throw new Error("rate must be 44100 or 22050");
  const parts = [];
  for (const loop of loops) {
    const samples = loop.samples;
    const adpcm = loop.adpcm || imaEncode(samples);
    const ns = loop.nsamples || samples.length;
    const bars = loop.bars;
    if (ns < 64 || ((ns + 1) >> 1) !== adpcm.length) throw new Error(`${loop.name}: ADPCM length is not the whole loop`);
    if (![1, 2, 4, 8].includes(bars)) throw new Error(`${loop.name}: bars must be 1, 2, 4 or 8`);
    const rec = new Uint8Array(20 + adpcm.length);
    const name = String(loop.name || "LOOP").slice(0, 7);
    for (let i = 0; i < name.length; i++) rec[i] = name.charCodeAt(i) & 0x7F;
    const dv = new DataView(rec.buffer);
    dv.setUint32(8, ns, true);
    dv.setUint8(12, bars);
    dv.setUint32(16, adpcm.length, true);
    rec.set(adpcm, 20);
    parts.push(rec);
  }
  const body = new Uint8Array(parts.reduce((n, p) => n + p.length, 0));
  let o = 0;
  for (const p of parts) { body.set(p, o); o += p.length; }
  const out = new Uint8Array(20 + body.length);
  for (let i = 0; i < 8; i++) out[i] = "X0XBRKPK".charCodeAt(i);
  const hv = new DataView(out.buffer);
  hv.setUint16(8, 1, true);
  hv.setUint16(10, loops.length, true);
  hv.setUint32(12, rate, true);
  hv.setUint32(16, packCrc32(body), true);
  out.set(body, 20);
  return out;
}

// Full WAV, no silence cut and no bar cut. Resampled to 44100. The bar count is the same rule as the firmware.
export function loopFromWav(buf, name) {
  const { rate, x } = readWav(buf);
  const samples = cut(x, rate, false, RATE, false);
  if (samples.length < 64) throw new Error(`${name} is too short`);
  const bars = barsOf(samples.length / RATE)[0];
  const clean = String(name || "LOOP").replace(/\.[^.]+$/, "").replace(/[^\x20-\x7e]/g, "").slice(0, 7) || "LOOP";
  return { name: clean, samples, bars, rate: RATE };
}
