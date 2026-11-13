// SPDX-License-Identifier: GPL-3.0-only
// DnBOX W3: local-only file preview. No MIDI, USB, device storage, or network calls.
import { PresetError, PresetSession, parseDnpr, ensureExportable, fileNameFor, sampleWarnings } from "./codec.mjs";
import { readDnprZip, makeDnprZip, ZIP_MAX_INPUT } from "./zip.mjs";

const $ = id => document.getElementById(id);
const session = new PresetSession(32);
let pending = [], selectedId = null, readToken = 0;

function status(message, type = "") {
  $("status").textContent = message;
  $("status").dataset.type = type;
}
function errorOf(err) {
  const code = err instanceof PresetError ? err.code : "BAD FILE";
  const details = err instanceof Error ? err.message : String(err);
  status(code + ". " + details + " No browser-session entries or keyboard data were changed.", "error");
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
    warnings.unshift("Original engine differs from record engine. Import to a target engine is blocked unless both agree.");
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
        const warningGroup = document.createElement("section"), label = document.createElement("strong");
        warningGroup.className = "preview-warning-record";
        label.textContent = "Record " + (index + 1) + " of " + pending.length + ": " + name + " (" + record.title + ")";
        warningGroup.append(label, notice); warnings.append(warningGroup);
      }
    }
    warnings.hidden = warnings.children.length === 0;
  }
  $("preview-count").textContent = pending.length === 1
    ? "One validated record. Keeping it does not send to a keyboard."
    : pending.length + " validated records: " + pending.map(p => (p.record.name || "Unnamed") + " (" + p.record.title + ")").join(", ") +
      ". All will enter the temporary session together, or none will.";
}
async function inspectFiles(files) {
  const token = ++readToken;
  pending = [];
  $("preview").hidden = true; $("preview-empty").hidden = false;
  if (!files.length) return status("Choose at least one DNPR file.");
  status("Checking files locally. No keyboard connection is used.");
  try {
    const prepared = [];
    for (const file of files) {
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
    if (session.entries.length + prepared.length > session.capacity)
      throw new PresetError("MEMORY FULL", "Browser session is full; no files were added");
    const target = options();
    pending = prepared.map(f => ({ name: f.name, bytes: f.bytes, record: parseDnpr(f.bytes, {
      ...target, fileName: f.name
    }) }));
    renderPending();
    status("All " + pending.length + " record(s) passed file checks. Review and explicitly keep them to continue.", "ok");
  } catch (err) { if (token === readToken) { clearPending(); errorOf(err); } }
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
  const a = document.createElement("a");
  a.href = url; a.download = name;
  document.body.append(a); a.click(); a.remove();
  setTimeout(() => URL.revokeObjectURL(url), 30000);
}
function uniqueFilesForZip() {
  const used = new Set();
  return session.entries.map(entry => {
    const b = ensureExportable(entry.record);
    const base = fileNameFor(entry.record, entry.id);
    let name = base, counter = 2;
    while (used.has(name.toLowerCase())) {
      const suffix = "." + entry.record.engine + ".dnpr";
      const stem = base.slice(0, -suffix.length);
      const mark = "-" + counter++;
      name = stem.slice(0, 64 - suffix.length - mark.length) + mark + suffix;
    }
    used.add(name.toLowerCase());
    return { name, bytes:b };
  });
}
$("files").addEventListener("change", event => inspectFiles([...event.target.files]));
$("model").addEventListener("change", () => clearPending("Target model changed. Choose files again for a fresh check."));
$("target").addEventListener("change", () => clearPending("Target engine changed. Choose files again for a fresh check."));
$("discard").addEventListener("click", () => clearPending("Preview discarded; nothing was changed."));
$("keep").addEventListener("click", () => {
  try {
    const entries = session.importMany(pending.map(p => ({ name:p.name, bytes:p.bytes })), options());
    selectedId = entries[0].id;
    clearPending();
    renderSession();
    status("Added " + entries.length + " validated record(s) to this browser session. No keyboard data changed.", "ok");
  } catch (err) { errorOf(err); }
});
$("save-file").addEventListener("click", () => {
  try {
    const item = session.exportOne(selectedId);
    download(item.bytes, item.name, "application/octet-stream");
    status("Download requested for " + item.name + ". The browser controls where the file is saved.", "ok");
  } catch (err) { errorOf(err); }
});
$("save-zip").addEventListener("click", () => {
  try {
    const files = uniqueFilesForZip();
    download(makeDnprZip(files), "DnBOX-user-presets.dnpr.zip", "application/zip");
    status("ZIP download requested for " + files.length + " unchanged preset(s). No device transfer took place.", "ok");
  } catch (err) { errorOf(err); }
});
$("clear").addEventListener("click", () => {
  if (!window.confirm("Clear the temporary browser session? Files on disk and the keyboard will not change.")) return;
  session.entries.length = 0;
  selectedId = null; renderSession();
  status("Browser session cleared. The keyboard and downloaded files are unchanged.");
});
const drop = $("drop");
drop.addEventListener("dragover", event => { event.preventDefault(); drop.classList.add("over"); });
drop.addEventListener("dragleave", () => drop.classList.remove("over"));
drop.addEventListener("drop", event => {
  event.preventDefault(); drop.classList.remove("over");
  inspectFiles([...event.dataTransfer.files]);
});
renderSession();