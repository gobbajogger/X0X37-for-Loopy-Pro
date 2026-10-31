// SPDX-License-Identifier: GPL-3.0-only
import {integer} from './model.js';

export const SCALES = Object.freeze({
  chromatic: Object.freeze([0,1,2,3,4,5,6,7,8,9,10,11]),
  major: Object.freeze([0,2,4,5,7,9,11]),
  minor: Object.freeze([0,2,3,5,7,8,10]),
  dorian: Object.freeze([0,2,3,5,7,9,10]),
  pentatonic: Object.freeze([0,2,4,7,9]),
  blues: Object.freeze([0,3,5,6,7,10])
});

export function scaleContains(pitch, root, scale) {
  if (!integer(pitch, 0, 127) || !integer(root, 0, 11) || !Object.hasOwn(SCALES, scale))
    throw Error('Choose a MIDI pitch, root and supported scale.');
  return SCALES[scale].includes((pitch - root + 12) % 12);
}

export function snapPitch(pitch, root, scale) {
  scaleContains(pitch, root, scale);
  for (let distance = 0; distance < 128; distance++) {
    if (pitch - distance >= 0 && scaleContains(pitch - distance, root, scale)) return pitch - distance;
    if (pitch + distance <= 127 && scaleContains(pitch + distance, root, scale)) return pitch + distance;
  }
}

export function snapStep(start, grid = 1) {
  if (!integer(start, 0, 63) || ![1,2,4,8].includes(grid)) throw Error('Invalid stored step or time grid.');
  return Math.min(Math.floor(63 / grid) * grid, Math.round(start / grid) * grid);
}
