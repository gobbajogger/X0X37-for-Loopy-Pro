// SPDX-License-Identifier: GPL-3.0-only
// DnBOX W3: local-only file preview. No MIDI, USB, device storage, or network calls.
import { PresetError, PresetSession, parseDnpr, ensureExportable, fileNameFor, sampleWarnings } from "./codec.mjs";
import { readDnprZip, makeDnprZip, ZIP_MAX_INPUT } from "./zip.mjs";

const $ = id => document.getElementById(id);
const session = new PresetSession(32);
let pending = [], selectedId = null, readToken = 0;

function status(message, type = "", region = "") {
  $("status").textContent = message;
  $("status").dataset.type = type;
  for (const id of ["preview-status", "session-status"]) {
    const local = $(id);
    local.hidden = id !== region;
    if (id === region) { local.textContent = message; local.dataset.type = type; }
  }
}
function errorOf(err, region = "") {
  const code = err instanceof PresetError ? err.code : "BAD FILE";
  const details = err instanceof Error ? err.message : String(err);
  status(code + ". " + details + " No browser-session entries or keyboard data were changed.", "error", region);
}
function options() {
  return { expectedEngine: $("target").value || null, targetModel: $("model").value || null };
}
function originName(v) {
  return ["Unknown", "Factory", "User", "Imported"][v] || "Unknown";
}
function appendMeta(element, record) {
  const fields = [
    ["Name", record.name || "Unnamed"],
    ["Engine", record.title + " (" + record.engine + ")"],
    ["Format", "DNPR v" + record.version + ", engine schema v" + record.schema],
    ["Integrity", "CRC-32 verified · " + record.bytes.length + " bytes"],
    ["Origin", originName(record.origin) + (record.factoryIndex ? " · derived #" + record.factoryIndex : "")],
    ["Original engine", record.originalEngine ? (record.originalEngine === record.engineId ? record.title : "Other engine ID " + record.originalEngine) : "Unknown"],
    ["Date", record.date ? String(record.date) : "Not recorded"],
    ["Source", record.sourceName || "Not recorded"],
    ["Samples", String(record.sampleRefs.length) + " references, not embedded audio"],
    ["Plugin metadata", record.pluginFormat ? "ID " + record.pluginFormat + " / v" + record.pluginVersion + " (unverified)" : "None"],
    ["Hardware", "Not encoded in DNPR v1"],
    ["Export", record.isFactoryDerived ? "Blocked: factory export OFF" : "Available as an unchanged user file"]
  ];
  element.replaceChildren();
  for (const [label, value] of fields) {
    const div = document.createElement("div"), dt = document.createElement("dt"), dd = document.createElement("dd");
    dt.textContent = label; dd.textContent = value; div.append(dt, dd); element.append(div);
  }
}
function appendWarnings(el, record) {
  const warnings = sampleWarnings(record);
  if (record.originalEngine && record.originalEngine !== record.engineId)
    warnings.unshift("Original engine differs from record engine. It is retained as provenance. Target checks use the actual record engine; no conversion is performed.");
  if (record.isFactoryDerived)
    warnings.unshift("FACTORY EXPORT OFF. You can inspect this file, but cannot download or send factory-derived bytes.");
  if (!warnings.length) { el.hidden = true; el.replaceChildren(); return; }
  const title = document.createElement("strong"), list = document.createElement("ul");
  title.textContent = "Before using this preset";
  for (const warning of warnings) {
    const item = document.createElement("li"); item.textContent = warning; list.append(item);
  }
  el.replaceChildren(title, list); el.hidden = false;
}
function clearPending(message = "") {
  ++readToken;
  pending = [];
  $("keep").disabled = false;
  $("preview").hidden = true; $("preview-empty").hidden = false;
  $("preview-meta").replaceChildren(); $("preview-warnings").replaceChildren();
  $("files").value = "";
  if (message) status(message);
}
function renderPending() {
  if (!pending.length) return clearPending();
  $("preview-empty").hidden = true; $("preview").hidden = false;
  const meta = $("preview-meta"), warnings = $("preview-warnings");
  if (pending.length === 1) {
    meta.className = "meta";
    appendMeta(meta, pending[0].record);
    appendWarnings(warnings, pending[0].record);
  } else {
    meta.className = "preview-records";
    meta.replaceChildren(); warnings.replaceChildren();
    for (const [index, { record }] of pending.entries()) {
      const name = record.name || "Unnamed";
      const heading = document.createElement("h3"), group = document.createElement("section"),
        details = document.createElement("dl"), notice = document.createElement("div");
      heading.textContent = "Record " + (index + 1) + " of " + pending.length + ": " + name + " (" + record.title + ")";
      group.className = "preview-record";
      details.className = "meta";
      appendMeta(details, record);
      group.append(heading, details); meta.append(group);
      appendWarnings(notice, record);
      if (!notice.hidden) {
        notice.className = "warning";
        group.append(notice);
      }
    }
    warnings.hidden = true;
  }
  $("preview-count").textContent = pending.length === 1
    ? "One validated record. Keeping it does not send to a keyboard."
    : pending.length + " validated records: " + pending.map(p => (p.record.name || "Unnamed") + " (" + p.record.title + ")").join(", ") +
      ". All will enter the temporary session together, or none will.";
}
async function inspectFiles(files) {
  const token = ++readToken;
  if (!files.length) {
    $("keep").disabled = false;
    return status("Choose at least one DNPR file.");
  }
  // Keep the last valid preview intact until the entire replacement validates.
  // It must not be kept while a new selection is still being checked.
  $("keep").disabled = true;
  status("Checking files locally. No keyboard connection is used.");
  let sourceName = "";
  try {
    const prepared = [];
    for (const file of files) {
      sourceName = file.name;
      if (!/\.(dnpr|zip)$/i.test(file.name))
        throw new PresetError("BAD FILE", "Unsupported file extension; choose .dnpr or .zip");
      if (file.size > ZIP_MAX_INPUT) throw new PresetError("BAD FILE", "Input exceeds 2 MiB");
      const bytes = new Uint8Array(await file.arrayBuffer());
      if (token !== readToken) return;
      const records = /\.zip$/i.test(file.name) ? await readDnprZip(bytes) : [{ name: file.name, bytes }];
      prepared.push(...records);
      if (prepared.length > session.capacity) throw new PresetError("MEMORY FULL", "More than 32 files in one browser session");
    }
    if (token !== readToken) return;
    sourceName = "";
    if (session.entries.length + prepared.length > session.capacity)
      throw new PresetError("MEMORY FULL", "Browser session is full; no files were added");
    const target = options();
    pending = prepared.map(f => {
      sourceName = f.name;
      return { name: f.name, bytes: f.bytes, record: parseDnpr(f.bytes, {
        ...target, fileName: f.name
      }) };
    });
    renderPending();
    status("All " + pending.length + " record(s) passed file checks. Review and explicitly keep them to continue.", "ok");
    $("preview-title").focus();
  } catch (err) {
    if (token === readToken) {
      $("files").value = "";
      const details = err instanceof Error ? err.message : String(err);
      const code = err instanceof PresetError ? "" : "BAD FILE: ";
      status(code + (sourceName ? sourceName + ": " : "") + details +
        " No browser-session entries or keyboard data were changed." +
        (pending.length ? " The previous validated preview is still available." : ""),
        "error", pending.length ? "preview-status" : "");
    }
  } finally {
    if (token === readToken) $("keep").disabled = false;
  }
}
function selectedEntry() { return session.entries.find(e => e.id === selectedId) || null; }
function renderSession(restoreFocus = false) {
  const root = $("session-list");
  root.replaceChildren();
  $("session-empty").hidden = session.entries.length > 0;
  $("save-zip").disabled = !session.entries.length;
  $("clear").disabled = !session.entries.length;
  const selected = selectedEntry();
  $("save-file").disabled = !selected;
  $("session-detail").hidden = !selected;
  if (selected) {
    appendMeta($("selected-meta"), selected.record);
    appendWarnings($("selected-warnings"), selected.record);
  }
  let selectedButton = null;
  for (const entry of session.entries) {
    const button = document.createElement("button"), label = document.createElement("b"), info = document.createElement("span");
    button.type = "button"; button.className = "record";
    button.setAttribute("aria-pressed", String(entry.id === selectedId));
    label.textContent = entry.record.name || "Unnamed preset";
    info.textContent = entry.record.title + " · " + entry.record.bytes.length + " B";
    button.append(label, info);
    button.addEventListener("click", () => { selectedId = entry.id; renderSession(true); });
    if (entry.id === selectedId) selectedButton = button;
    root.append(button);
  }
  if (restoreFocus && selectedButton) selectedButton.focus({ preventScroll:true });
}
function download(bytes, name, mime) {
  const url = URL.createObjectURL(new Blob([bytes], { type:mime }));
  let a = null, requested = false;
  try {
    a = document.createElement("a");
    a.href = url; a.download = name;
    document.body.append(a); a.click();
    requested = true;
  } finally {
    try { if (a) a.remove(); }
    finally {
      if (requested) setTimeout(() => URL.revokeObjectURL(url), 30000);
      else URL.revokeObjectURL(url);
    }
  }
}
function exportError(err, downloadAttempted = false) {
  const details = err instanceof Error ? err.message : String(err);
  status("EXPORT FAILED. " + details + ". " + (downloadAttempted ? "Check browser downloads before retrying. " : "") +
    "Browser-session entries are unchanged.", "error", "session-status");
}
function uniqueFilesForZip() {
  const used = new Set(), files = [];
  let skipped = 0;
  for (const entry of session.entries) {
    let bytes;
    try { bytes = ensureExportable(entry.record); }
    catch (err) {
      if (err instanceof PresetError && err.code === "FACTORY EXPORT OFF") { skipped++; continue; }
      throw err;
    }
    const base = fileNameFor(entry.record, entry.id);
    let name = base, counter = 2;
    while (used.has(name.toLowerCase())) {
      const suffix = "." + entry.record.engine + ".dnpr";
      const stem = base.slice(0, -suffix.length);
      const mark = "-" + counter++;
      name = stem.slice(0, 64 - suffix.length - mark.length) + mark + suffix;
    }
    used.add(name.toLowerCase());
    files.push({ name, bytes });
  }
  return { files, skipped };
}
$("files").addEventListener("change", event => inspectFiles([...event.target.files]));
$("model").addEventListener("change", () => clearPending("Target model changed. Choose files again for a fresh check."));
$("target").addEventListener("change", () => clearPending("Target engine changed. Choose files again for a fresh check."));
$("discard").addEventListener("click", () => {
  clearPending("Preview discarded; nothing was changed.");
  $("files").focus();
});
$("keep").addEventListener("click", () => {
  if (!pending.length || $("keep").disabled) return;
  try {
    const entries = session.importMany(pending.map(p => ({ name:p.name, bytes:p.bytes })), options());
    selectedId = entries[0].id;
    clearPending();
    renderSession(true);
    status("Added " + entries.length + " validated record(s) to this browser session. No keyboard data changed.", "ok", "session-status");
  } catch (err) { errorOf(err, "preview-status"); }
});
$("save-file").addEventListener("click", () => {
  try {
    const item = session.exportOne(selectedId);
    try { download(item.bytes, item.name, "application/octet-stream"); }
    catch (err) { return exportError(err, true); }
    status("Download requested for " + item.name + ". The browser controls where the file is saved.", "ok", "session-status");
  } catch (err) {
    if (err instanceof PresetError && err.code === "FACTORY EXPORT OFF") errorOf(err, "session-status");
    else exportError(err);
  }
});
$("save-zip").addEventListener("click", () => {
  try {
    const { files, skipped } = uniqueFilesForZip();
    if (!files.length) {
      status("No eligible records to download. " + skipped + " factory or factory-derived record(s) remain in this browser session.", "error", "session-status");
      return;
    }
    const bytes = makeDnprZip(files);
    try { download(bytes, "DnBOX-user-presets.dnpr.zip", "application/zip"); }
    catch (err) { return exportError(err, true); }
    status("ZIP download requested for " + files.length + " unchanged eligible preset(s). " + skipped +
      " factory or factory-derived record(s) excluded and retained in this session. The browser controls where the file is saved.", "ok", "session-status");
  } catch (err) { exportError(err); }
});
$("clear").addEventListener("click", () => {
  if (!window.confirm("Clear the temporary browser session? Files on disk and the keyboard will not change.")) return;
  session.entries.length = 0;
  selectedId = null; renderSession();
  $("files").focus();
  status("Browser session cleared. The keyboard and downloaded files are unchanged.", "", "session-status");
});
const drop = $("drop");
drop.addEventListener("dragover", event => { event.preventDefault(); drop.classList.add("over"); });
drop.addEventListener("dragleave", () => drop.classList.remove("over"));
drop.addEventListener("drop", event => {
  event.preventDefault(); drop.classList.remove("over");
  inspectFiles([...event.dataTransfer.files]);
});
renderSession();
