// SPDX-License-Identifier: GPL-3.0-only
// DnBOX DNPR v1 browser codec. File-only: no device transport or plugin parser.
// Wire source: firmware/src/app/preset_record.[ch] at 16f9f9ad.
export const DNPR_MAX_BYTES = 548;
export const DNPR_ENGINES = Object.freeze({
  1: Object.freeze({ token: "chicago", title: "Chicago", params: 22, samples: 0, factory: 81 }),
  2: Object.freeze({ token: "miami", title: "Miami", params: 18, samples: 0, factory: 30 }),
  3: Object.freeze({ token: "juno", title: "Juno", params: 18, samples: 0, factory: 4 }),
  4: Object.freeze({ token: "drone", title: "Drone", params: 8, samples: 0, factory: 4 }),
  5: Object.freeze({ token: "909", title: "909", params: 132, samples: 4, factory: 0 }),
  6: Object.freeze({ token: "808", title: "808", params: 144, samples: -1, factory: 0 }),
  7: Object.freeze({ token: "break", title: "BREAK", params: 16, samples: 2, factory: 0 })
});
export const DNPR_EXPORT_FACTORY = false;

export class PresetError extends Error {
  constructor(code, detail = "") {
    super(code + (detail ? ": " + detail : ""));
    this.name = "PresetError";
    this.code = code;
  }
}
const fail = (code, detail) => { throw new PresetError(code, detail); };
const le16 = (b, at) => b[at] | (b[at + 1] << 8);
const le32 = (b, at) => (b[at] | b[at + 1] << 8 | b[at + 2] << 16 | b[at + 3] << 24) >>> 0;
const put16 = (b, at, v) => { b[at] = v & 255; b[at + 1] = (v >>> 8) & 255; };
const put32 = (b, at, v) => { for (let i = 0; i < 4; ++i) b[at + i] = (v >>> (8 * i)) & 255; };
const uint = (n, max) => Number.isInteger(n) && n >= 0 && n <= max;
const toBytes = (value) => {
  if (value instanceof Uint8Array) return new Uint8Array(value);
  if (value instanceof ArrayBuffer) return new Uint8Array(value.slice(0));
  fail("BAD FILE", "Expected binary DNPR bytes");
};
const engineNumber = (id) => {
  if (typeof id === "number") return DNPR_ENGINES[id] ? id : null;
  return Number(Object.keys(DNPR_ENGINES).find(k => DNPR_ENGINES[k].token === String(id).toLowerCase())) || null;
};
// Names are opaque C byte strings. Escape controls for display, never edit original bytes.
const readPad = (b, start, length) => {
  let end = start;
  while (end < start + length && b[end] !== 0) ++end;
  if (end === start + length) fail("BAD FILE", "Unterminated name");
  for (let i = end + 1; i < start + length; ++i)
    if (b[i] !== 0) fail("BAD FILE", "Nonzero name padding");
  let shown = "";
  for (let i = start; i < end; ++i)
    shown += (b[i] >= 32 && b[i] <= 126) ? String.fromCharCode(b[i]) : "\uFFFD";
  return shown;
};
const writePad = (b, start, length, value) => {
  const s = String(value ?? "");
  if (s.length >= length || /[^\x20-\x7E]/.test(s))
    fail("BAD FILE", "New names must be printable ASCII, at most " + (length - 1) + " bytes");
  for (let i = 0; i < s.length; ++i) b[start + i] = s.charCodeAt(i);
};
export function crc32(bytes, zeroCrcField = false) {
  let c = 0xffffffff;
  for (let i = 0; i < bytes.length; ++i) {
    c ^= (zeroCrcField && i >= 16 && i < 20) ? 0 : bytes[i];
    for (let bit = 0; bit < 8; ++bit) c = ((c >>> 1) ^ ((c & 1) ? 0xedb88320 : 0)) >>> 0;
  }
  return (c ^ 0xffffffff) >>> 0;
}
export function fileNameFor(record, sequence = 1) {
  const id = engineNumber(record.engineId ?? record.engine);
  if (!id) fail("WRONG ENGINE");
  const suffix = "." + DNPR_ENGINES[id].token + ".dnpr";
  const fallback = "user-" + String(sequence).padStart(2, "0");
  let stem = String(record.name || fallback).normalize("NFKD")
    .replace(/[^A-Za-z0-9 _-]/g, "").trim().replace(/\s+/g, "-")
    .replace(/[-_]+/g, "-").replace(/^[-_]+/, "");
  stem = stem.slice(0, 64 - suffix.length).replace(/[-_]+$/, "") || fallback;
  return stem + suffix;
}
export function explicitModelHint(filename) {
  const m = String(filename).toLowerCase().match(/\.(elite|pro)\.(chicago|miami|juno|drone|909|808|break)\.dnpr$/);
  return m ? m[1] : null;
}
export function ensureDeviceModel(requested, reported) {
  if (!["elite", "pro"].includes(requested) || !["elite", "pro"].includes(reported))
    fail("DEVICE UNVERIFIED", "DNPR v1 itself does not encode the hardware model");
  if (requested !== reported) fail("WRONG MODEL", "Connected device reports " + reported);
  return true;
}
export function parseDnpr(input, options = {}) {
  const b = toBytes(input);
  if (b.length < 100 || b.length > DNPR_MAX_BYTES) fail("BAD FILE", "Record size outside DNPR v1 limits");
  if (le32(b, 0) !== 0x52504e44) fail("BAD FILE", "Not a DNPR record");
  const total = le32(b, 12);
  if (total !== b.length || total < 100 || total > DNPR_MAX_BYTES) fail("BAD FILE", "Length mismatch");
  const checksum = le32(b, 16);
  if (crc32(b, true) !== checksum) fail("BAD FILE", "CRC mismatch");
  const version = le16(b, 4), schema = le16(b, 8), flags = le16(b, 10);
  if (version > 1 || schema > 1) fail("NEWER DATA", "Unsupported version or engine schema");
  if (version !== 1 || schema !== 1 || flags !== 0 || le16(b, 98) !== 0)
    fail("BAD FILE", "Unsupported version, flags or reserved bytes");
  const engineId = le16(b, 6), spec = DNPR_ENGINES[engineId];
  if (!spec) fail("WRONG ENGINE", "Unknown engine ID");
  const expectedId = options.expectedEngine == null ? null : engineNumber(options.expectedEngine);
  if (options.expectedEngine != null && !expectedId) fail("WRONG ENGINE", "Unknown target engine");
  const origin = b[20], originalEngine = b[21], factoryIndex = le16(b, 22);
  if (origin > 3 || (originalEngine !== 0 && !DNPR_ENGINES[originalEngine]))
    fail("BAD FILE", "Invalid provenance fields");
  if (factoryIndex > spec.factory || (origin === 1 && factoryIndex === 0))
    fail("BAD FILE", "Invalid factory provenance");
  // Like native preset_record_import, target eligibility follows the actual engine.
  // Recognized originalEngine remains provenance; it does not authorize conversion.
  if (expectedId && engineId !== expectedId)
    fail("WRONG ENGINE", "Target differs from actual record engine");
  const hint = explicitModelHint(options.fileName || "");
  if (options.targetModel != null && !["elite", "pro"].includes(options.targetModel))
    fail("DEVICE UNVERIFIED", "Unknown target model");
  if (hint && options.targetModel && hint !== options.targetModel)
    fail("WRONG MODEL", "Filename hint conflicts with selected model; device is still unverified");
  const nparams = b[96], nsamples = b[97];
  if (nparams !== spec.params || (spec.samples >= 0 ? nsamples !== spec.samples : nsamples > 16) ||
      100 + nparams + nsamples * 16 !== total)
    fail("BAD FILE", "Engine parameter or sample-reference count");
  const name = readPad(b, 32, 32), sourceName = readPad(b, 64, 32);
  const sampleRefs = [];
  for (let i = 0; i < nsamples; ++i) {
    const off = 100 + nparams + i * 16;
    const source = b[off], index = le16(b, off + 2), crc = le32(b, off + 4);
    if (b[off + 1] !== 0 || source > 3 || (source === 0 && (index !== 0 || crc !== 0)))
      fail("BAD FILE", "Invalid sample reference " + (i + 1));
    sampleRefs.push(Object.freeze({ source, index, crc, name: readPad(b, off + 8, 8) }));
  }
  const isFactoryDerived = origin === 1 || factoryIndex !== 0;
  return Object.freeze({
    bytes: b, checksum, version, schema, engineId, engine: spec.token, title: spec.title,
    name, sourceName, origin, originalEngine, factoryIndex, isFactoryDerived,
    date: le32(b, 24), pluginFormat: le16(b, 28), pluginVersion: le16(b, 30),
    params: b.slice(100, 100 + nparams), sampleRefs: Object.freeze(sampleRefs),
    modelInRecord: false, filenameModelHint: hint
  });
}
export function encodeDnpr(fields) {
  const engineId = engineNumber(fields.engineId ?? fields.engine), spec = DNPR_ENGINES[engineId];
  if (!spec) fail("WRONG ENGINE");
  if (!(fields.params instanceof Uint8Array) || fields.params.length !== spec.params)
    fail("BAD FILE", "Explicit " + spec.params + "-byte engine parameter payload required");
  const refs = fields.sampleRefs || [];
  if (!Array.isArray(refs) || (spec.samples >= 0 ? refs.length !== spec.samples : refs.length > 16))
    fail("BAD FILE", "Wrong number of sample references");
  const b = new Uint8Array(100 + spec.params + 16 * refs.length);
  put32(b, 0, 0x52504e44); put16(b, 4, 1); put16(b, 6, engineId); put16(b, 8, 1);
  put32(b, 12, b.length);
  b[20] = fields.origin ?? 2; b[21] = fields.originalEngine ?? engineId;
  put16(b, 22, fields.factoryIndex ?? 0);
  put32(b, 24, fields.date ?? 0);
  put16(b, 28, fields.pluginFormat ?? 0); put16(b, 30, fields.pluginVersion ?? 0);
  writePad(b, 32, 32, fields.name ?? "");
  writePad(b, 64, 32, fields.sourceName ?? "");
  b[96] = spec.params; b[97] = refs.length;
  b.set(fields.params, 100);
  for (let i = 0; i < refs.length; ++i) {
    const off = 100 + spec.params + i * 16, s = refs[i];
    if (!uint(s.source, 3) || !uint(s.index, 65535) || !uint(s.crc, 0xffffffff))
      fail("BAD FILE", "Invalid sample reference");
    b[off] = s.source; put16(b, off + 2, s.index); put32(b, off + 4, s.crc);
    writePad(b, off + 8, 8, s.name ?? "");
  }
  put32(b, 16, crc32(b, true));
  parseDnpr(b);
  return b;
}
export function ensureExportable(record) {
  if (record.isFactoryDerived || record.origin === 1 || record.factoryIndex > 0)
    fail("FACTORY EXPORT OFF", "Factory and factory-derived records cannot be downloaded");
  return new Uint8Array(record.bytes);
}
export function sampleWarnings(record, inventory = null) {
  if (record.engine === "808" && record.sampleRefs.length === 0)
    return ["No sample references are recorded; the kit cannot be verified and may be incomplete."];
  return record.sampleRefs.flatMap((r, i) => {
    if (r.source === 0) return ["Sample " + (i + 1) + " is unassigned; the kit may be incomplete."];
    const key = r.source + ":" + r.index;
    const actual = inventory && Object.hasOwn(inventory, key) ? inventory[key] : null;
    if (actual == null) return ["Sample " + (i + 1) + " (" + key + ") not checked against keyboard; it may be missing or changed."];
    if (actual !== r.crc) return ["Sample " + (i + 1) + " (" + key + ") missing / changed (CRC mismatch)."];
    return [];
  });
}
export class PresetSession {
  constructor(capacity = 32) {
    if (!uint(capacity, 128) || capacity === 0) fail("BAD FILE", "Invalid browser session capacity");
    this.capacity = capacity; this.entries = []; this.nextId = 1;
  }
  // Atomic host staging: decode every file and check capacity before inserting any.
  importMany(files, options = {}) {
    if (!Array.isArray(files) || files.length === 0) fail("BAD FILE", "Select a DNPR file");
    const parsed = files.map(f => ({ filename: String(f.name || ""), record: parseDnpr(f.bytes, {
      expectedEngine: options.expectedEngine, targetModel: options.targetModel, fileName: f.name
    }) }));
    if (this.entries.length + parsed.length > this.capacity) fail("MEMORY FULL", "Browser session is full; no entries changed");
    const added = parsed.map(p => Object.freeze({ id: this.nextId++, filename: p.filename, record: p.record }));
    this.entries.push(...added);
    return added;
  }
  exportOne(id) {
    const p = this.entries.find(e => e.id === id);
    if (!p) fail("BAD FILE", "Selected preset not found");
    return { name: fileNameFor(p.record, p.id), bytes: ensureExportable(p.record) };
  }
}
