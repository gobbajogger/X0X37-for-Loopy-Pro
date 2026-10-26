// SPDX-License-Identifier: GPL-3.0-only
// Visual regression contract: narrow-screen steps stay readable and unmodified install guards remain present.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';

const html = readFileSync(process.argv[2] || new URL('./x0x_installer.html', import.meta.url), 'utf8');
const media = html.split('@media (max-width: 530px) {')[1]?.split('@media (prefers-reduced-motion: reduce)')[0];
assert.ok(media, 'expected existing narrow-screen media rule');
const steps = media.match(/\.journey li \{([^}]*)\}/)?.[1];
assert.ok(steps, 'expected narrow-screen journey step styles');
assert.match(steps, /font:\s*650\s+12px\/1\.4/, 'steps cannot shrink to unreadable 9px text');
assert.match(steps, /flex-direction:\s*column/, 'step number and text must fit the narrow column');
assert.match(steps, /overflow-wrap:\s*anywhere/, 'translated labels must not force horizontal overflow');
assert.match(html, /<ol class="journey"/, 'guided order remains semantic');
assert.match(html, /<button id="go" disabled/, 'public firmware install stays gated');
assert.match(html, /<button id="stockgo" disabled/, 'local firmware install stays gated');
assert.match(html, /<button class="local-pick" id="chooseLocal" type="button"/, 'local firmware CTA retained');
console.log('PASS narrow guided steps readable, translated labels wrap and install controls remain gated');
