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
export function watchDeviceSession(access, input, output, invalidate) {
  let live = true;
  const check = () => {
    if (live && (input.state === 'disconnected' || output.state === 'disconnected' ||
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
