// SPDX-License-Identifier: GPL-3.0-only
// Six-stage installer projection: structure, translations and the stage/result rules.
// Ported from the archived installer lane (0da4e7da, d0abbd96). No MIDI, no writes.
// Run: node web/test_installer_six_stage_flow.mjs
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { Script } from "node:vm";

const html = readFileSync(new URL("./x0x_installer.html", import.meta.url), "utf8");
const ids = ["stepbrowser", "stepready", "stepconnect", "stepchoose", "stepinstall", "stepdone"];
const at = ids.map((id) => html.indexOf(`<li id="${id}"`));
assert.ok(at.every((i, n) => i > 0 && (n === 0 || i > at[n - 1])), "six ordered stage items");
for (const id of ["flowready", "flow-result", "connectbar", "status"]) assert.ok(html.includes(`id="${id}"`), id);
assert.ok(html.indexOf('id="flowready"') < html.indexOf('id="connectbar"'), "preparation before connection");
assert.ok(html.indexOf('id="status"') < html.indexOf('id="flow-result"'), "result follows live status");

const i = html.indexOf("// BEGIN SIX_STAGE_PROJECTION"), j = html.indexOf("function refreshJourney()", i);
assert.ok(i > 0 && j > i);
const { flowStage, flowResultKey } = new Script(html.slice(i, j) + ";({flowStage, flowResultKey})").runInNewContext({});
const base = { capable: true, writing: false, connected: false, scanning: false, outcome: "waiting", selected: false };
assert.equal(flowStage({ ...base, capable: false }), 0);
assert.equal(flowStage({ ...base, capable: false, writing: true }), 0);
assert.equal(flowStage(base), 1);
assert.equal(flowStage({ ...base, scanning: true }), 2);
assert.equal(flowStage({ ...base, connected: true }), 3);
assert.equal(flowStage({ ...base, connected: true, selected: true }), 4);
assert.equal(flowStage({ ...base, writing: true, outcome: "complete" }), 4, "busy wins over a stale result");
assert.equal(flowStage({ ...base, connected: true, outcome: "complete" }), 5);
assert.equal(flowStage({ ...base, connected: true, selected: true, outcome: "incomplete" }), 4, "incomplete never reaches Done");
assert.equal(flowResultKey("complete", true), "flowBusy");
assert.equal(flowResultKey("complete", false), "flowComplete");
assert.equal(flowResultKey("incomplete", false), "flowIncomplete");
assert.equal(flowResultKey("waiting", false), "flowWaiting");

// run() sets the outcome only from final results, and resets it before any write.
const run = html.slice(html.indexOf("async function run("), html.indexOf('$("go").addEventListener("click"'));
assert.ok(run.indexOf('flowOutcome = "waiting"') < run.indexOf("await midiAccess()"));
assert.match(run, /flowOutcome = ok \? "complete" : "incomplete"/);
assert.match(run, /flowOutcome = r\.move && r\.move\.skipped && r\.move\.skipped\.length \? "incomplete" : "complete"/);
assert.match(run, /led\("err"\);\n    flowOutcome = "incomplete";/);
assert.ok(!/step\("done"[^)]*\)[^;]*flowOutcome/.test(html), "updater step(done) never sets Done");

for (const k of ["flow1", "flow2", "flow6", "flowFirst", "flowBackupProof", "flowResultHelp", "flowBusy", "flowComplete", "flowIncomplete", "overflow", "badreq"]) {
  assert.ok(new RegExp(`"${k}":`).test(html), k);
}
console.log("installer six-stage flow: ok");
