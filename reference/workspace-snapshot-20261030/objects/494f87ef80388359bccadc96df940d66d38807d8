// SPDX-License-Identifier: GPL-3.0-only
// F02: after a commanded loader transition, waitFor must not attach to another keyboard.
// No real MIDI, no device write. Models the audited first-match bug and asserts the
// fail-closed newly-appeared rule for web/fm1ota.js.
// Run from the repo root: node web/test_f02_loader_bind.mjs
import { Updater, pack7 } from "./fm1ota.js";

let bad = 0;
const ok = (c, what) => { console.log(`${what.padEnd(72)} ${c ? "ok" : "FAIL"}`); if (!c) bad++; };

function identityPacket(text) {
  const t = [...new TextEncoder().encode(text)];
  if (t.length > 27) throw new Error("identity text does not fit");
  const payload = [...t, ...new Array(27 - t.length).fill(0)];
  const body = [0, 0x59, 0x11, 27, 0, 0, ...payload, ~payload.reduce((sum, b) => sum + b, 0) & 255];
  return [0xF0, ...pack7(body), 0xF7];
}

const HS = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const eq = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);

/** Read-only WebMIDI access: each named port answers the live identity in `devices`. */
function makeAccess(devices) {
  const inputs = new Map(), outputs = new Map();
  for (const name of Object.keys(devices)) {
    const id = Math.random().toString(36).slice(2);
    const input = { id: "i" + id, name, state: "connected", onmidimessage: null, open: async () => {} };
    const output = {
      id: "o" + id, name, state: "connected", open: async () => {},
      send: (d) => {
        if (!eq(Array.from(d), HS)) return;
        const bytes = identityPacket(devices[name]);
        setTimeout(() => {
          if (input.state === "connected" && input.onmidimessage) {
            input.onmidimessage({ data: Uint8Array.from(bytes) });
          }
        }, 1);
      },
    };
    inputs.set(input.id, input);
    outputs.set(output.id, output);
  }
  return { inputs, outputs };
}

const IS_OTA = (id) => /^ota-/i.test(id.model);

async function main() {
  // Other keyboard already in its update loader and enumerated first; selected SMK still running.
  const devices = {
    "Other OTA": "ota-FM-1_906",
    "Selected SMK": "SMK-37 Pro_900",
  };
  const access = makeAccess(devices);
  const up0 = new Updater(access);
  const wrong = await up0.find(IS_OTA);
  ok(wrong && wrong.name === "Other OTA", "model: first-match would attach to the unrelated loader");
  if (wrong) wrong.link.close();

  const up = new Updater(access);
  const before = await up.matchingPorts(IS_OTA);
  ok(before.length === 1 && before[0] === "Other OTA", "snapshot before transition is the pre-existing loader only");

  const none = await up.waitFor(IS_OTA, 50, before);
  ok(none === null, "waitFor: only a pre-existing loader -> timeout (noloader), not that port");

  devices["Selected SMK"] = "ota-SMK-37 Pro_900";
  const got = await up.waitFor(IS_OTA, 2000, before);
  ok(got && got.name === "Selected SMK" && IS_OTA(got.id),
     "waitFor: accepts exactly the newly appeared loader, not Other OTA");
  if (got) got.link.close();
  ok(up.returnedPort === "Selected SMK", "returnedPort records the accepted transition port");

  const access2 = makeAccess({
    "Other OTA": "ota-FM-1_906",
    "Felucca New A": "ota-SMK-37 Pro_900",
    "Felucca New B": "ota-SMK-37 Elite_900",
  });
  const up2 = new Updater(access2);
  const err = await up2.waitFor(IS_OTA, 50, ["Other OTA"]).then(() => null, (e) => e);
  ok(err && err.code === "model" && /more than one new device/.test(err.message),
     "waitFor: two new loaders throw (fail closed)");

  const access3 = makeAccess({
    "Other SMK": "SMK-37 Pro_800",
    "Selected SMK": "SMK-37 Pro_900",
  });
  const up3 = new Updater(access3);
  up3.returnedPort = "Selected SMK";
  const back = await up3.reconnectAfter((id) => !IS_OTA(id), 2000);
  ok(back && back.name === "Selected SMK",
     "reconnectAfter: reopens the transition port, not the sibling keyboard");
  if (back) back.link.close();

  process.exit(bad ? 1 : 0);
}

main().catch((e) => { console.error(e); process.exit(1); });
