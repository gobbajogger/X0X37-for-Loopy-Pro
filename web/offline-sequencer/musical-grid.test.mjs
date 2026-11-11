// SPDX-License-Identifier: GPL-3.0-only
import test from 'node:test';
import assert from 'node:assert/strict';
import {snapPitch, snapStep, scaleContains} from './musical-grid.js';

test('C major snaps chromatic notes to the nearest member, with lower ties', () => {
  assert.equal(snapPitch(61, 0, 'major'), 60);
  assert.equal(snapPitch(63, 0, 'major'), 62);
  assert.equal(snapPitch(65, 0, 'major'), 65);
  assert.equal(scaleContains(66, 0, 'major'), false);
});
test('scale membership follows the root across octaves and MIDI boundaries', () => {
  assert.equal(snapPitch(0, 1, 'major'), 0); // B-sharp belongs to C-sharp major.
  assert.equal(snapPitch(0, 2, 'major'), 1);
  assert.equal(snapPitch(127, 0, 'major'), 127);
  assert.equal(snapPitch(127, 1, 'major'), 126);
  assert.equal(scaleContains(69, 9, 'minor'), true);
  assert.equal(scaleContains(70, 9, 'minor'), false);
  assert.equal(snapPitch(61, 0, 'chromatic'), 61);
});
test('time snap never wraps the final stored step or accepts malformed grids', () => {
  assert.equal(snapStep(3, 2), 4);
  assert.equal(snapStep(63, 4), 60);
  assert.equal(snapStep(0, 8), 0);
  for (const invalid of [-1, 64, NaN, 1.5]) assert.throws(() => snapStep(invalid, 1));
  for (const invalid of [0, 3, NaN]) assert.throws(() => snapStep(4, invalid));
});
test('untrusted root, pitch and scale cannot silently change a note', () => {
  assert.throws(() => snapPitch(60, 12, 'major'));
  assert.throws(() => snapPitch(60.5, 0, 'major'));
  assert.throws(() => snapPitch(60, 0, 'unknown'));
  assert.throws(() => scaleContains(60, -1, 'major'));
});
