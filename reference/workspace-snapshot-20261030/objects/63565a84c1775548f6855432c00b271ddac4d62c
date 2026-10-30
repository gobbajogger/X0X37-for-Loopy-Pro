// SPDX-License-Identifier: GPL-3.0-only
// Boot screen record for the installer page. The bytes match firmware/src/app/boot_screen.h.
// A save is not done until the keyboard's END reply is 0. Anything refused leaves the previous screen.
import { pack78, unpack78, slotReq } from "./x0x_breaks.js";

export const BOOT_LINE0 = 12, BOOT_LINE1 = 21, BOOT_W = 240, BOOT_H = 96;
export const BOOT_PIC_BYTES = (BOOT_W * BOOT_H) / 8;
export const BOOT_FIXED = 44;
export const BOOT_FILE_MAX = 8 * 1024 * 1024;
const KIND_CUSTOM = 1, KIND_FACTORY = 2, F_PIC = 1;
const CMD = { info: 39, begin: 43, write: 44, read: 45, end: 46, abort: 47 };

function bad(code) { return { ok: false, code }; }

function lineBytes(s, max) {
  const t = String(s ?? "");
  if (t.length > max) return null;
  const out = new Uint8Array(max);
  for (let i = 0; i < t.length; i++) {
    const c = t.charCodeAt(i);
    if (c < 32 || c > 126) return null;
    out[i] = c;
  }
  return { n: t.length, bytes: out };
}

// {ok, bytes} or {ok:false, code}. codes: short, long0, long1, chars, picsize.
export function bootPack({ factory = false, line0 = "", line1 = "", bits = null } = {}) {
  if (factory) {
    const rec = new Uint8Array(BOOT_FIXED);
    rec[0] = KIND_FACTORY;
    return { ok: true, bytes: rec };
  }
  if (!String(line0).length || !String(line1).length) return bad("short");
  if (String(line0).length > BOOT_LINE0) return bad("long0");
  if (String(line1).length > BOOT_LINE1) return bad("long1");
  const a = lineBytes(line0, BOOT_LINE0), b = lineBytes(line1, BOOT_LINE1);
  if (!a || !b) return bad("chars");
  if (!a.n || !b.n) return bad("short");
  const pic = bits != null;
  if (pic && bits.length !== BOOT_PIC_BYTES) return bad("picsize");
  const rec = new Uint8Array(BOOT_FIXED + (pic ? BOOT_PIC_BYTES : 0));
  rec[0] = KIND_CUSTOM;
  rec[1] = pic ? F_PIC : 0;
  rec[2] = a.n;
  rec[3] = b.n;
  rec.set(a.bytes, 4);
  rec.set(b.bytes, 16);
  if (pic) {
    rec[40] = BOOT_W & 255;
    rec[41] = BOOT_W >> 8;
    rec[42] = BOOT_H & 255;
    rec[43] = BOOT_H >> 8;
    rec.set(bits, BOOT_FIXED);
  }
  return { ok: true, bytes: rec };
}

// 240×96 RGBA (straight alpha) -> 1-bit, amber ink on black. null if the buffer is the wrong size.
export function bootRgbaToBits(rgba) {
  if (!rgba || rgba.length !== BOOT_W * BOOT_H * 4) return null;
  const bits = new Uint8Array(BOOT_PIC_BYTES), stride = BOOT_W / 8;
  for (let y = 0; y < BOOT_H; y++) {
    for (let x = 0; x < BOOT_W; x++) {
      const i = (y * BOOT_W + x) * 4;
      const lum = rgba[i] * 0.299 + rgba[i + 1] * 0.587 + rgba[i + 2] * 0.114;
      if (rgba[i + 3] >= 128 && lum >= 128)
        bits[y * stride + (x >> 3)] |= 0x80 >> (x & 7);
    }
  }
  return bits;
}

export function bootBitsToRgba(bits) {
  const rgba = new Uint8ClampedArray(BOOT_W * BOOT_H * 4);
  if (!bits || bits.length !== BOOT_PIC_BYTES) return rgba;
  const stride = BOOT_W / 8;
  for (let y = 0; y < BOOT_H; y++) {
    for (let x = 0; x < BOOT_W; x++) {
      const on = bits[y * stride + (x >> 3)] & (0x80 >> (x & 7));
      const i = (y * BOOT_W + x) * 4;
      if (on) { rgba[i] = 232; rgba[i + 1] = 136; rgba[i + 2] = 30; }
      rgba[i + 3] = 255;
    }
  }
  return rgba;
}

function textFrom(bytes, max) {
  let s = "";
  for (let i = 0; i < bytes.length && i < max && bytes[i]; i++) s += String.fromCharCode(bytes[i]);
  return s;
}

// Device reply after the command byte. Does not claim success unless END is 0.
export async function bootTransfer(link, bytes) {
  let began = false, ended = false;
  const abort = async () => {
    try { await slotReq(link, CMD.abort, [], 1000, 1); } catch (_) { /* the stage also times out */ }
  };
  try {
    let r = await slotReq(link, CMD.begin, [], 5000);
    if (!r.length || r[0]) return { ok: false, code: r[0] || 1, ended: false };
    began = true;
    for (let off = 0; off < bytes.length; off += 256) {
      const chunk = bytes.subarray(off, Math.min(bytes.length, off + 256));
      r = await slotReq(link, CMD.write, [off & 127, (off >> 7) & 127, (off >> 14) & 127, ...pack78(chunk)]);
      if (r.length < 4 || r[3]) {
        await abort();
        return { ok: false, code: r.length >= 4 ? r[3] : 1, ended: false };
      }
    }
    ended = true;
    r = await slotReq(link, CMD.end, [], 15000);
    if (!r.length || r[0]) return { ok: false, code: r[0] || 1, ended: true };
    return { ok: true, ended: true };
  } catch (e) {
    if (began && !ended) await abort();
    return { ok: false, code: ended ? "unconfirmed" : "lost", ended, error: e };
  }
}

export async function bootRead(link) {
  const r = await slotReq(link, CMD.info, [], 3000);
  if (!r.length) throw new Error("short");
  let i = 2;
  const take = () => {
    let s = "";
    while (i < r.length && r[i]) s += String.fromCharCode(r[i++]);
    i++;
    return s;
  };
  const status = r[0], flags = r[1] || 0;
  const line0 = take(), line1 = take();
  let bits = null;
  if (status !== 2 && flags & F_PIC) {
    bits = new Uint8Array(BOOT_PIC_BYTES);
    for (let off = 0; off < BOOT_PIC_BYTES;) {
      const want = Math.min(224, BOOT_PIC_BYTES - off), adr = BOOT_FIXED + off;
      const part = await slotReq(link, CMD.read, [adr & 127, (adr >> 7) & 127, (adr >> 14) & 127, (want + 1) >> 1]);
      if (!part.length || part[0]) throw new Error("read");
      const raw = unpack78(part.slice(4));
      const n = Math.min(raw.length, BOOT_PIC_BYTES - off);
      bits.set(raw.subarray(0, n), off);
      off += n;
      if (!n) throw new Error("read");
    }
  }
  return { status, line0, line1, bits };
}
