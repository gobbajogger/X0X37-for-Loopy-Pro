"use strict";

const dom = {
  actor: document.getElementById("actorStatus"),
  form: document.getElementById("ownerForm"),
  build: document.getElementById("build"),
  model: document.getElementById("model"),
  stage: document.getElementById("stage"),
  when: document.getElementById("when"),
  note: document.getElementById("note"),
  quote: document.getElementById("quote"),
  source: document.getElementById("source"),
  package: document.getElementById("packageInfo"),
  preview: document.getElementById("previewOutput"),
  commit: document.getElementById("commitButton"),
  previewButton: document.getElementById("previewButton"),
  reset: document.getElementById("resetButton"),
  gates: document.getElementById("gateList"),
  candidateButton: document.getElementById("candidateButton"),
  candidateOutput: document.getElementById("candidateOutput"),
  detail: document.getElementById("technicalDetail"),
};
let session = null;
let previewId = null;

function text(node, value) {
  node.textContent = String(value);
}
function chosen() {
  return session?.records.find((r) => r.catalog_id === dom.build.value) || null;
}
function chosenPackage() {
  return chosen()?.models?.[dom.model.value] || null;
}
function clearPreview() {
  previewId = null;
  dom.commit.disabled = true;
  text(dom.preview, "No accepted preview. A new preview is required after every change.");
}
function showRecord() {
  clearPreview();
  const rec = chosen();
  if (!rec) {
    text(dom.package, "No build selected.");
    text(dom.gates, "");
    text(dom.detail, "");
    dom.previewButton.disabled = true;
    return;
  }
  const pkg = chosenPackage();
  const blocked = rec.withdrawn || !pkg || !session?.owner_verified;
  dom.previewButton.disabled = blocked;
  text(dom.package, rec.withdrawn
    ? "Withdrawn build. New test reports are disabled."
    : pkg ? "Selected exact " + dom.model.value + " package: " + pkg.package_sha256
          : "No package for this model.");
  text(dom.candidateOutput, "No promotion evaluation requested.");
  dom.gates.replaceChildren();
  const gates = rec.gates || {};
  for (const [name, status] of Object.entries(gates)) {
    const line = document.createElement("div");
    line.className = "gate";
    const gate = document.createElement("span");
    const result = document.createElement("strong");
    gate.textContent = name;
    result.textContent = status;
    result.className = ["pass", "missing", "fail"].includes(status) ? status : "";
    line.append(gate, result);
    dom.gates.append(line);
  }
  const summaries = [];
  for (const [model, identity] of Object.entries(rec.models || {})) {
    summaries.push(model + ":\n  package: " + identity.package_sha256
                 + "\n  image: " + identity.image_sha256);
  }
  text(dom.detail, "Build: " + rec.catalog_id + "\n" + summaries.join("\n"));
}

async function send(path, payload) {
  const response = await fetch(path, {
    method: "POST",
    mode: "same-origin",
    cache: "no-store",
    credentials: "same-origin",
    headers: {
      "Content-Type": "application/json",
      "X-DnBOX-CSRF": session.csrf || "",
    },
    body: JSON.stringify(payload),
  });
  const json = await response.json();
  if (!response.ok) {
    throw new Error(json.error || json.errors?.join("\n") || ("HTTP " + response.status));
  }
  return json;
}

async function load() {
  const response = await fetch("/api/status", {cache:"no-store",credentials:"same-origin"});
  if (!response.ok) throw new Error("Status unavailable: HTTP " + response.status);
  session = await response.json();
  const allowed = !!session.owner_verified;
  dom.form.querySelectorAll("select,input,textarea").forEach((input) => {input.disabled = !allowed;});
  dom.build.replaceChildren();
  for (const record of session.records) {
    const opt = document.createElement("option");
    opt.value = record.catalog_id;
    opt.textContent = record.version + " · " + record.catalog_id +
                      (record.withdrawn ? " (withdrawn)" : "");
    opt.disabled = !!record.withdrawn;
    dom.build.append(opt);
  }
  const first = session.records.find((record) => !record.withdrawn);
  if (first) dom.build.value = first.catalog_id;
  dom.when.value = new Date().toISOString().replace(".000Z", "Z");
  dom.actor.className = allowed ? "status good" : "status bad";
  text(dom.actor, allowed
    ? "Owner session verified through server-side GitHub login. " +
      (session.commit_enabled ? "Commit mode enabled once for the pinned feature branch." :
       "Dry-run only. Saving is disabled.")
    : "Owner GitHub identity not verified. Read-only status; outcome editing disabled.");
  showRecord();
}

dom.candidateButton.addEventListener("click", async () => {
  const rec = chosen();
  if (!rec) return;
  dom.candidateButton.disabled = true;
  try {
    const response = await fetch("/api/promotion-candidate?catalog_id=" +
      encodeURIComponent(rec.catalog_id), {cache:"no-store"});
    const report = await response.json();
    if (!response.ok) throw new Error(report.error || "Dry-run endpoint failed");
    text(dom.candidateOutput, [
      report.gate_pass ? "Evidence gates passed (no authority)." :
                         "Not ready. Missing gates:",
      ...(report.blocking_gates || []).map(v => "• " + v),
      "Promotion authority: " + report.deployment_authority,
      "Promotion performed: NO",
    ].join("\n"));
  } catch (err) {
    text(dom.candidateOutput, "Dry-run unavailable: " + err.message);
  } finally {
    dom.candidateButton.disabled = false;
  }
});

dom.build.addEventListener("change", showRecord);
dom.model.addEventListener("change", showRecord);
dom.form.addEventListener("input", clearPreview);
dom.stage.addEventListener("change", clearPreview);
dom.reset.addEventListener("click", clearPreview);
dom.form.addEventListener("submit", async (event) => {
  event.preventDefault();
  clearPreview();
  if (!session?.owner_verified) return;
  const rec = chosen();
  const pkg = chosenPackage();
  if (!rec || !pkg || rec.withdrawn) return;
  const entry = {
    catalog_id: rec.catalog_id,
    model: dom.model.value,
    package_sha256: pkg.package_sha256,
    stage: dom.stage.value,
    observed_at: dom.when.value.trim(),
    note: dom.note.value.trim(),
  };
  if (dom.quote.value.trim()) {
    entry.quoted_words = dom.quote.value.trim();
    entry.source_link = dom.source.value.trim();
  }
  dom.previewButton.disabled = true;
  try {
    const result = await send("/api/preview", entry);
    const detail = [
      "RECORD " + result.record_id,
      "Model: " + result.model,
      "Exact selected package: " + result.package_sha256,
      "Event: " + result.stage,
      "Evidence: " + result.confidence,
      "Hardware approved: NO",
      "Firmware promoted: NO",
      "",
      result.accepted ? "Validation accepted. No files changed." :
                        "NOT ACCEPTED\n" + (result.errors || []).join("\n"),
    ];
    text(dom.preview, detail.join("\n"));
    if (result.accepted) previewId = result.preview_id;
    dom.commit.disabled = !(result.accepted && session.commit_enabled);
  } catch (err) {
    text(dom.preview, "Preview failed: " + err.message);
  } finally {
    dom.previewButton.disabled = false;
  }
});

dom.commit.addEventListener("click", async () => {
  if (!previewId || !session?.commit_enabled) return;
  if (!window.confirm("Commit this exact model-specific OWNER-REPORTED outcome to PR #883? No firmware publication or hardware approval occurs.")) return;
  dom.commit.disabled = true;
  const nonce = previewId;
  previewId = null;
  try {
    const result = await send("/api/commit", {preview_id: nonce});
    text(dom.preview, "Saved to existing feature branch.\nCommit: " + result.commit +
      "\nHardware approval: NO\nPromotion: not-performed");
    await load();
  } catch (err) {
    text(dom.preview, "Commit not confirmed: " + err.message +
      "\nReload status and inspect the branch before retrying.");
  }
});

load().catch((err) => {
  dom.actor.className = "status bad";
  text(dom.actor, err.message);
  dom.previewButton.disabled = true;
  dom.commit.disabled = true;
});
