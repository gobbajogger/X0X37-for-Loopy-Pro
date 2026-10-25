// SPDX-License-Identifier: GPL-3.0-only
// F02 regression for web/fm1ota.js waitFor: no real MIDI, no device write.
// An unrelated loader enumerated first must not be chosen after a transition.
import { Updater, pack7 } from "./fm1ota.js";

let failed = 0;
const ok = (cond, what) => {
  console.log(`${what.padEnd(72)} ${cond ? "ok" : "FAIL"}`);
  if (!cond) failed++;
};

function identityPacket(text) {
  const t = [...new TextEncoder().encode(text)];
  const body = [0, 0x59, 0x11, 0, 0, 0, ...t, ...new Array(28 - t.length).fill(0)];
  return [0xF0, ...pack7(body), 0xF7];
}

/** WebMIDI access stub: named ports that answer the handshake with fixed identities. */
function makeAccess(devices) {
  const access = { inputs: new Map(), outputs: new Map() };
  for (const name of Object.keys(devices)) {
    const id = name.replace(/\W+/g, "");
    const input = {
      id: "i" + id, name, state: "connected", onmidimessage: null,
      open: async () => {},
    };
    const output = {
      id: "o" + id, name, state: "connected",
      open: async () => {},
      send: () => {
        const pkt = identityPacket(devices[name]);
        setTimeout(() => {
          if (input.state === "connected" && input.onmidimessage) {
            input.onmidimessage({ data: Uint8Array.from(pkt) });
          }
        }, 1);
      },
    };
    access.inputs.set(input.id, input);
    access.outputs.set(output.id, output);
  }
  return access;
}

const IS_OTA = (id) => /^ota-/i.test(id.model);

async function main() {
  const devices = {
    "Other OTA": "ota-FM-1_906",
    "Selected SMK": "SMK-37 Pro_900",
  };
  const up = new Updater(makeAccess(devices));

  // Empty known (no pre-transition snapshot): the unrelated loader is accepted.
  const legacy = await up.waitFor(IS_OTA, 50, new Set());
  ok(legacy !== null && legacy.name === "Other OTA",
    "legacy empty-known: first loader is the unrelated keyboard (bug shape)");
  if (legacy) legacy.link.close();

  const known = await up.matchingNames(IS_OTA);
  ok(known.size === 1 && known.has("Other OTA"),
    "snapshot before transition includes the unrelated loader");
  const bound = await up.waitFor(IS_OTA, 50, known);
  ok(bound === null, "bound wait ignores the pre-existing loader (fail closed, no write)");

  devices["Selected SMK"] = "ota-SMK-37 Pro_900";
  const got = await up.waitFor(IS_OTA, 2000, known);
  ok(got !== null && got.name === "Selected SMK" && /^ota-SMK/i.test(got.id.model),
    "bound wait accepts the one newly appeared loader");
  if (got) got.link.close();

  devices["Elite OTA"] = "ota-SMK-37 Elite_900";
  const up3 = new Updater(makeAccess(devices));
  let amb = null;
  try {
    await up3.waitFor(IS_OTA, 2000, new Set(["Other OTA"]));
  } catch (e) { amb = e; }
  ok(amb && amb.code === "ambiguous", "two new loaders raise ambiguous");

  // Post-write: other running keyboard must not win the reboot wait.
  const knownRun = new Set(["Other FM-1"]);
  const after = {
    "Other FM-1": "FM-1_906",
    "Selected SMK Back": "SMK-37 Pro_900",
  };
  const back = await new Updater(makeAccess(after)).waitFor((id) => !IS_OTA(id), 2000, knownRun);
  ok(back !== null && back.name === "Selected SMK Back",
    "bound reboot wait accepts the one newly appeared running keyboard");
  if (back) back.link.close();

  console.log(failed ? `F02 WEB LOADER BIND TESTS FAILED (${failed})` : "f02 web loader bind tests passed");
  process.exit(failed ? 1 : 0);
}

main();
