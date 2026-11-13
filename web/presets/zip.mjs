// SPDX-License-Identifier: GPL-3.0-only
// Offline host-only ZIP container for individual DNPR v1 records. No bank wire format.
import { crc32, PresetError, DNPR_MAX_BYTES } from "./codec.mjs";
const fail = (code, detail) => { throw new PresetError(code, detail); };
const bytes = b => b instanceof Uint8Array ? b : new Uint8Array(b);
const u16 = (b, at) => b[at] | b[at + 1] << 8;
const u32 = (b, at) => (b[at] | b[at + 1] << 8 | b[at + 2] << 16 | b[at + 3] << 24) >>> 0;
const w16 = (b, at, n) => { b[at] = n & 255; b[at + 1] = n >>> 8 & 255; };
const w32 = (b, at, n) => { for (let i = 0; i < 4; ++i) b[at + i] = n >>> (8 * i) & 255; };
export const ZIP_MAX_INPUT = 2 * 1024 * 1024;
export const ZIP_MAX_ENTRIES = 64;

function nameBytes(name) {
  if (!/^[\x20-\x7e]{1,128}$/.test(name) || /[\\/:]/.test(name) ||
      name === "." || name === ".." || !/\.dnpr$/i.test(name))
    fail("BAD FILE", "ZIP must contain flat .dnpr filenames only");
  return Uint8Array.from(name, c => c.charCodeAt(0));
}
function parseName(raw) {
  if (raw.some(c => c < 32 || c > 126)) fail("BAD FILE", "ZIP filename must be ASCII");
  const name = String.fromCharCode(...raw);
  nameBytes(name);
  return name;
}
function bound(bytesLength, at, n) {
  if (!Number.isSafeInteger(at) || !Number.isSafeInteger(n) || at < 0 || n < 0 || at + n > bytesLength)
    fail("BAD FILE", "ZIP record outside archive");
}
async function inflateLimited(compressed, expected) {
  if (typeof DecompressionStream !== "function")
    fail("UNSUPPORTED ZIP", "Deflate is unavailable in this browser; use a stored ZIP or individual DNPR files");
  let stream;
  try { stream = new Blob([compressed]).stream().pipeThrough(new DecompressionStream("deflate-raw")); }
  catch { fail("UNSUPPORTED ZIP", "Deflate-raw is unavailable; use individual DNPR files"); }
  const chunks = [], reader = stream.getReader();
  let size = 0;
  try {
    for (;;) {
      const part = await reader.read();
      if (part.done) break;
      size += part.value.length;
      if (size > DNPR_MAX_BYTES || size > expected) {
        await reader.cancel();
        fail("BAD FILE", "ZIP entry expands beyond declared preset size");
      }
      chunks.push(part.value);
    }
  } catch (err) {
    if (err instanceof PresetError) throw err;
    fail("BAD FILE", "Invalid compressed preset");
  } finally { reader.releaseLock(); }
  if (size !== expected) fail("BAD FILE", "ZIP uncompressed-size mismatch");
  const out = new Uint8Array(size);
  let i = 0;
  for (const part of chunks) { out.set(part, i); i += part.length; }
  return out;
}
export async function readDnprZip(input) {
  const b = bytes(input);
  if (b.length > ZIP_MAX_INPUT || b.length < 22) fail("BAD FILE", "ZIP size out of range");
  let eocd = -1;
  for (let i = b.length - 22; i >= Math.max(0, b.length - 65557); --i) {
    if (u32(b, i) === 0x06054b50 && i + 22 + u16(b, i + 20) === b.length) { eocd = i; break; }
  }
  if (eocd < 0) fail("BAD FILE", "ZIP directory is missing");
  if (u16(b, eocd + 4) !== 0 || u16(b, eocd + 6) !== 0 ||
      u16(b, eocd + 8) !== u16(b, eocd + 10))
    fail("UNSUPPORTED ZIP", "Multi-disk ZIP is not supported");
  const count = u16(b, eocd + 10), cdLen = u32(b, eocd + 12), cdAt = u32(b, eocd + 16);
  if (count === 0 || count > ZIP_MAX_ENTRIES || cdAt + cdLen !== eocd)
    fail("BAD FILE", "ZIP directory size/count mismatch");
  const names = new Set(), entries = [];
  let at = cdAt;
  for (let i = 0; i < count; ++i) {
    bound(b.length, at, 46);
    if (u32(b, at) !== 0x02014b50) fail("BAD FILE", "Invalid ZIP directory entry");
    const flags = u16(b, at + 8), method = u16(b, at + 10);
    const wantedCrc = u32(b, at + 16), compressedSize = u32(b, at + 20),
      size = u32(b, at + 24), nlen = u16(b, at + 28),
      extra = u16(b, at + 30), comment = u16(b, at + 32), local = u32(b, at + 42);
    if (flags & 1 || flags & ~0x0808 || ![0, 8].includes(method) ||
        size < 100 || size > DNPR_MAX_BYTES || compressedSize > ZIP_MAX_INPUT)
      fail("UNSUPPORTED ZIP", "Encrypted, oversized or unsupported ZIP member");
    bound(b.length, at + 46, nlen + extra + comment);
    const name = parseName(b.subarray(at + 46, at + 46 + nlen));
    if (names.has(name.toLowerCase())) fail("BAD FILE", "Duplicate ZIP filename");
    names.add(name.toLowerCase());
    bound(b.length, local, 30);
    if (u32(b, local) !== 0x04034b50 || u16(b, local + 8) !== method ||
        u16(b, local + 6) !== flags) fail("BAD FILE", "ZIP local header conflict");
    const localNameLen = u16(b, local + 26), localExtra = u16(b, local + 28);
    bound(b.length, local + 30, localNameLen + localExtra);
    const localName = parseName(b.subarray(local + 30, local + 30 + localNameLen));
    if (localName !== name) fail("BAD FILE", "ZIP filename mismatch");
    const dataAt = local + 30 + localNameLen + localExtra;
    if (dataAt + compressedSize > cdAt) fail("BAD FILE", "ZIP member overlaps directory");
    const packed = b.subarray(dataAt, dataAt + compressedSize);
    if (method === 0 && compressedSize !== size) fail("BAD FILE", "Stored entry length mismatch");
    const payload = method === 0 ? new Uint8Array(packed) : await inflateLimited(packed, size);
    if (crc32(payload) !== wantedCrc) fail("BAD FILE", "ZIP member CRC mismatch: " + name);
    entries.push({ name, bytes: payload });
    at += 46 + nlen + extra + comment;
  }
  if (at !== eocd) fail("BAD FILE", "Unexpected trailing ZIP directory data");
  return entries;
}
export function makeDnprZip(files) {
  if (!Array.isArray(files) || !files.length || files.length > ZIP_MAX_ENTRIES)
    fail("BAD FILE", "Choose between 1 and " + ZIP_MAX_ENTRIES + " presets");
  const seen = new Set(), members = [], central = [];
  let position = 0, centralSize = 0;
  for (const f of files) {
    const name = nameBytes(String(f.name));
    const key = String(f.name).toLowerCase();
    if (seen.has(key)) fail("BAD FILE", "Duplicate ZIP filename");
    seen.add(key);
    const data = bytes(f.bytes);
    if (data.length < 100 || data.length > DNPR_MAX_BYTES) fail("BAD FILE", "DNPR size out of range");
    const check = crc32(data);
    const local = new Uint8Array(30 + name.length);
    w32(local, 0, 0x04034b50); w16(local, 8, 0);
    w16(local, 10, 0); w16(local, 12, 0x21); // DOS date: 1980-01-01
    w32(local, 14, check); w32(local, 18, data.length); w32(local, 22, data.length);
    w16(local, 26, name.length); local.set(name, 30);
    const cd = new Uint8Array(46 + name.length);
    w32(cd, 0, 0x02014b50); w16(cd, 4, 20); w16(cd, 6, 20);
    w16(cd, 14, 0x21); w32(cd, 16, check); w32(cd, 20, data.length);
    w32(cd, 24, data.length); w16(cd, 28, name.length); w32(cd, 42, position);
    cd.set(name, 46);
    members.push(local, data); central.push(cd);
    position += local.length + data.length; centralSize += cd.length;
  }
  const end = new Uint8Array(22);
  w32(end, 0, 0x06054b50); w16(end, 8, files.length);
  w16(end, 10, files.length); w32(end, 12, centralSize); w32(end, 16, position);
  const total = position + centralSize + end.length;
  if (total > ZIP_MAX_INPUT) fail("BAD FILE", "ZIP output exceeds limit");
  const out = new Uint8Array(total); let at = 0;
  for (const part of [...members, ...central, end]) { out.set(part, at); at += part.length; }
  return out;
}