// SPDX-License-Identifier: GPL-3.0-only
import test from 'node:test';
import assert from 'node:assert/strict';
import {encodeDnpr, parseDnpr, sampleWarnings} from './codec.mjs';

test('808 with no sample references warns that its kit cannot be verified', () => {
  const bytes = encodeDnpr({engine:'808',origin:2,name:'Empty kit',params:new Uint8Array(144),sampleRefs:[]});
  const record = parseDnpr(bytes);
  assert.match(sampleWarnings(record).join(' '), /no sample references.*(?:incomplete|not verified)/i);
});
