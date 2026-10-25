// SPDX-License-Identifier: GPL-3.0-only
// The browser pack matches tools/break_pack.py, and it lands in the free tail only.
import { execFileSync } from "node:child_process";
import { encodeBreakPack } from "./x0x_breaks.js";
import { placeBreakPack } from "./fm1pkg.js";

let failed = 0;
const ok = (cond, what) => { console.log(`${what.padEnd(64)} ${cond ? "ok" : "FAIL"}`); if (!cond) failed++; };

const samples = new Int16Array(80);
for (let i = 0; i < samples.length; i++) samples[i] = (i * 100) % 30000;
const js = encodeBreakPack([{ name: "TONE", samples, bars: 1 }]);
const py = execFileSync("python3", ["-c", `
import sys
sys.path.insert(0, "tools")
import break_pack as bp
from sampleio import ima_encode
samples = [(i * 100) % 30000 for i in range(80)]
adpcm, _ = ima_encode(samples, 0)
sys.stdout.buffer.write(bp.encode_pack([{"name": "TONE", "adpcm": adpcm, "nsamples": 80, "bars": 1}]))
`]);
ok(js.length === py.length && js.every((b, i) => b === py[i]), "JS pack matches Python");

const codeMax = 4096;
const app = new Uint8Array(codeMax);
app.fill(0xA5, 0, 100);
app[100] = 0;
app.fill(0xFF, 101);
placeBreakPack(app, js);
ok(app[0] === 0xA5 && app[99] === 0xA5 && app[100] === 0, "place leaves the image");
let at = -1;
for (let o = 0; o + 8 <= codeMax; o += 16) {
  if (app[o] === 0x58 && app[o + 1] === 0x30 && app[o + 2] === 0x58 && app[o + 3] === 0x42) at = o;
}
ok(at > 100 && at + js.length <= codeMax, "pack is in the free tail");
let same = at >= 0;
if (same) for (let i = 0; i < js.length; i++) if (app[at + i] !== js[i]) same = false;
ok(same, "tail bytes are the pack");
let threw = false;
try { placeBreakPack(new Uint8Array(32).fill(0xFF), js); } catch (e) { threw = true; }
ok(threw, "a pack bigger than the tail is refused");

if (failed) process.exit(1);
