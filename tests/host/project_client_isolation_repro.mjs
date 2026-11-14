// SPDX-License-Identifier: GPL-3.0-only
// Two actual ProjectDevice callers race against the real C editor dispatcher.
// The bridge gives each reply to its sending caller, an optimistic boundary that
// isolates transfer ownership from the separate uncorrelated-reply problem.
// A failing assertion is the open isolation defect, not a passing acceptance.
import assert from "node:assert/strict";
import { spawn } from "node:child_process";
import { createInterface } from "node:readline";
import { ProjectDevice, PROJECT_OP, readProjectSnapshot } from "../../web/x0x_projects.js";

const binary = process.argv[2];
assert(binary, "Pass the compiled project_client_dispatch path.");
const child = spawn(binary, [], { stdio: ["pipe", "pipe", "inherit"] });
const targets = [], trace = [];
const exited = new Promise((resolve, reject) => {
  child.once("error", reject);
  child.once("exit", (code, signal) => code === 0 ? resolve() : reject(new Error(`dispatcher exit ${code}/${signal}`)));
});
const lines = createInterface({ input: child.stdout });
lines.on("line", line => {
  const reply = line.split(" ").map(Number);
  trace.push(`reply op=${reply[5]} rc=${reply[6]}`);
  const target = targets.shift();
  assert(target, "one reply for each dispatched request");
  target.deliver(reply);
});
class Link {
  constructor(name) { this.name = name; this.queue = []; this.pending = null; this.gate = null; }
  drain() { this.queue.length = 0; }
  dispatch(frame) {
    targets.push(this);
    child.stdin.write(`${frame.join(" ")}\n`);
  }
  send(frame) {
    trace.push(`${this.name} send cmd=${frame[4]} op=${frame[4] === 9 ? frame[5] : "-"}`);
    if (frame[4] === 9 && this.gate?.op === frame[5]) {
      const gate = this.gate; this.gate = null;
      gate.release = () => this.dispatch(frame);
      gate.reached();
    } else this.dispatch(frame);
    return true;
  }
  read(timeout) {
    if (this.queue.length) return Promise.resolve(this.queue.shift());
    assert(!this.pending, "one outstanding read per caller");
    return new Promise(resolve => {
      const timer = setTimeout(() => { this.pending = null; resolve(null); }, timeout);
      this.pending = reply => { clearTimeout(timer); this.pending = null; resolve(reply); };
    });
  }
  deliver(reply) { if (this.pending) this.pending(reply); else this.queue.push(reply); }
  hold(op) {
    const gate = { op };
    gate.ready = new Promise(resolve => { gate.reached = resolve; });
    this.gate = gate;
    return gate;
  }
}
const aLink = new Link("A"), bLink = new Link("B");
const a = new ProjectDevice(aLink, { timeout: 3000 }), b = new ProjectDevice(bLink, { timeout: 3000 });
const causalControl = process.argv.includes("--control-omit-cleanup");
if (causalControl) b.abort = async () => { b.active = false; b.mode = 0; };
const outcome = promise => promise.then(value => ({ value }), error => ({ error }));
try {
  await a.connect(); await b.connect();
  // B sees idle, then pauses immediately before its BEGIN reaches firmware.
  const bBegin = bLink.hold(PROJECT_OP.EXPORT);
  const bAttempt = outcome(readProjectSnapshot(b));
  await bBegin.ready;
  // A acquires the hold. Pause its first read so B's delayed BEGIN sees busy.
  const aRead = aLink.hold(PROJECT_OP.READ);
  const aAttempt = outcome(a.exportStart());
  await aRead.ready;
  bBegin.release();
  const bResult = await bAttempt;
  assert.equal(bResult.error?.rc, 2, "B must receive the firmware busy rejection");
  aRead.release();
  const aResult = await aAttempt;
  assert.equal(aResult.error, undefined,
    `B's rejected BEGIN cleanup must preserve A's export; A got ${aResult.error?.code}: ${aResult.error?.message}`);
  assert.equal(aResult.value.length, a.projectLength);
  await a.abort();
  console.log(`${causalControl ? "causal control (B cleanup omitted)" : "project client isolation"}: rejected B BEGIN preserves actual A export`);
} catch (error) {
  console.error(trace.slice(-30).join("\n"));
  throw error;
} finally {
  child.stdin.end();
  await exited;
}
