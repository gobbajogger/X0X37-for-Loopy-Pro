// SPDX-License-Identifier: GPL-3.0-only
// #76: the installer must lead to the already versioned, protective project+song transfer.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
const dir = dirname(fileURLToPath(import.meta.url));
const installer = readFileSync(join(dir, 'x0x_installer.html'), 'utf8');
const projects = readFileSync(join(dir, 'x0x_projects.html'), 'utf8');
const protocol = readFileSync(join(dir, 'x0x_projects.js'), 'utf8');
const links = [...installer.matchAll(/<a\s+href="([^\"]+)"\s+data-t="projects">/g)];
assert.equal(links.length, 1, 'installer must have one translated project/song route');
assert.equal(new URL(links[0][1], 'https://example.org/X0X37-for-Loopy-Pro/install/').href,
             'https://example.org/X0X37-for-Loopy-Pro/projects/');
assert.match(installer, /data-t="projectshint"/);
assert.equal([...installer.matchAll(/\bprojects: "/g)].length, 2, 'ja/en names required');
assert.equal([...installer.matchAll(/\bprojectshint: "/g)].length, 2, 'ja/en explanations required');
assert.match(projects, /id="export"/);
assert.match(projects, /id="prepare"/);
assert.match(projects, /id="import"/);
assert.match(projects, /type="file" accept="\.x0xproject"/);
assert.match(projects, /import \{ PROJECT_MAX_FILE, ProjectDevice, ProjectMidiLink,/);
assert.match(protocol, /const PROJECT_MAGIC = "X0XPROJ\\0"/);
assert.match(protocol, /parseProject\(/);
assert.match(protocol, /prepareProjectImport/);
assert.match(protocol, /applyProjectImport/);
console.log('PASS installer project+song handoff: one local link, two languages, existing guarded project format');
