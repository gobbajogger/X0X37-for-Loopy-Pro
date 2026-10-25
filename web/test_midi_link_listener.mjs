// Web MIDI callback coexistence regression. Run: node web/test_midi_link_listener.mjs
// Executes the current Link class from fm1ota.js without opening a MIDI device.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { runInNewContext } from 'node:vm';

const src = readFileSync(new URL('./fm1ota.js', import.meta.url), 'utf8');
const start = src.indexOf('class Link {');
const end = src.indexOf('\nasync function handshake(', start);
// Permit the standalone source-window test fixture as well as the full source module.
assert.ok(start >= 0, 'Link class must exist');
const code = src.slice(start, end > start ? end : undefined);
const Link = runInNewContext(`${code}\nLink;`, { Promise, setTimeout, clearTimeout });
let assertions = 0;
const check = (actual, expected, message) => { assert.deepEqual(actual, expected, message); assertions++; };

class MIDIInputMock extends EventTarget {
  constructor() { super(); this.state = 'connected'; this.onmidimessage = null; }
  emit(bytes) {
    const ev = new Event('midimessage'); ev.data = Uint8Array.from(bytes);
    this.dispatchEvent(ev); this.onmidimessage?.(ev);
  }
}
const output = { state: 'connected', send() {} };
const sysex = [0xf0, 1, 0xf7], note = [0x90, 64, 96];

// Web MIDI's existing property callback must coexist with the new event listener.
const input = new MIDIInputMock();
let previousCalls = 0, observerCalls = 0;
const previous = () => previousCalls++;
input.onmidimessage = previous;
input.addEventListener('midimessage', () => observerCalls++);
const link = new Link(input, output);
check(input.onmidimessage, previous, 'must not replace Web MIDI property callback');
input.emit(sysex); input.emit(note);
check(previousCalls, 2, 'original handler still gets SysEx and notes');
check(observerCalls, 2, 'other event listeners still receive messages');
check(link.q.length, 1, 'installer queues only SysEx');
link.close(); link.close();
check(input.onmidimessage, previous, 'close cannot erase original property callback');
input.emit(sysex);
check(previousCalls, 3, 'original handler remains usable after installer closes');
check(observerCalls, 3, 'unrelated event listener remains attached');
check(link.q.length, 1, 'closed link cannot receive new events');

// Old property-only implementations remain supported without stealing handlers.
const legacy = { state: 'connected', onmidimessage: null };
let oldCalls = 0;
const old = () => oldCalls++;
legacy.onmidimessage = old;
const legacyLink = new Link(legacy, output);
legacy.onmidimessage({data:Uint8Array.from(sysex)});
check(oldCalls, 1, 'legacy handler still fires during installer connection');
check(legacyLink.q.length, 1, 'legacy link still collects SysEx');
legacyLink.close();
check(legacy.onmidimessage, old, 'legacy original callback restored on close');
const secondLegacyLink = new Link(legacy, output);
const replacement = () => {};
legacy.onmidimessage = replacement;
secondLegacyLink.close();
check(legacy.onmidimessage, replacement, 'close must not erase a newer callback');

// Closing one of two independent readers cannot detach the other.
const shared = new MIDIInputMock();
const first = new Link(shared, output), second = new Link(shared, output);
first.close(); shared.emit(sysex);
check(first.q.length, 0, 'first reader detached');
check(second.q.length, 1, 'second reader remains connected');
second.close();
console.log(JSON.stringify({status:'PASS', assertions, source:'fm1ota.js Link source-window', runtime:process.version}));
