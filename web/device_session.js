// SPDX-License-Identifier: GPL-3.0-only
// One browser-origin owner for MIDI operations, including same-origin workspaces/tabs.
// Other applications still need the firmware's transaction protocol; this lock cannot bind them.
export async function withDeviceOperation(action) {
  if (!globalThis.navigator?.locks?.request)
    throw new Error('Exclusive browser MIDI access is unavailable. Use a current Chrome or Edge over HTTPS or localhost. Offline preparation remains available.');
  return navigator.locks.request('dnbox-midi-operation', { ifAvailable: true }, async (lock) => {
    if (!lock) throw new Error('Another DnBOX workspace is using MIDI. Wait for it to finish, then reconnect and prepare the operation again.');
    return action();
  });
}

// Opening a port may emit statechange while it remains connected. Only a removed,
// disconnected or replaced target invalidates the session, even if its name/id is reused.
// Optional armed(): once it returns true (the session is identified), any statechange
// of a bound endpoint also invalidates it. A queued event may see a port that has
// already reconnected; the earlier implicit-open events precede identification.
export function watchDeviceSession(access, input, output, invalidate, armed = null) {
  let live = true;
  const bound = (port) => !!port && [input, output].some((b) => port === b ||
    (typeof port.id === 'string' && port.id && port.id === b.id &&
     (port.type === 'input' || port.type === 'output') && port.type === b.type));
  const check = (event) => {
    if (live && ((armed?.() && bound(event?.port)) ||
        input.state === 'disconnected' || output.state === 'disconnected' ||
        access.inputs.get(input.id) !== input || access.outputs.get(output.id) !== output)) {
      live = false;
      invalidate();
    }
  };
  access.addEventListener?.('statechange', check);
  return {
    assert() { check(); if (!live) throw new Error('The keyboard connection changed. Reconnect and prepare again; your local files and recovery backup are retained.'); },
    close() { live = false; access.removeEventListener?.('statechange', check); },
  };
}
