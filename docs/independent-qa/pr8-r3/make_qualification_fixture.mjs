// SPDX-License-Identifier: GPL-3.0-only
// Synthetic evidence/bytes exercise preparation; none represents a real approval.
import assert from 'node:assert/strict';
import { createHash, webcrypto } from 'node:crypto';
import { readFileSync } from 'node:fs';
import { Script } from 'node:vm';
import { validateCatalog, verifyDownloads, renderInstaller, installerFingerprint, requiredGates }
  from "file:///workspace/pr8-independent-qa/scripts/render-qualified-nightly.mjs";

const hash = b => createHash('sha256').update(b).digest('hex');
const raw = Buffer.alloc(1024, 0x7d), product = 'SMK-37 Pro_900';
for (let i = 0; i < product.length; i++) raw[i * 48 + 47] = (product.charCodeAt(i) + i + 1) & 255;
const digest = hash(raw), ctl = new Uint8Array([1, 2, 3]);
const evidence = 'https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/103';
const commit = '1'.repeat(40), tree = '2'.repeat(40);
const pkg = { product, sha256: digest, size: raw.length,
  controller_sha256: hash(ctl), pkg: `https://example.org/${digest}/pro.fwsc` };
const catalog = { schema: 1, latest: 'synthetic-test', history: [], versions: [{
  version: 'synthetic-test', label: 'DEV / HARDWARE TEST — DEVICE NOT TESTED',
  status: 'dev-checked', config: 'dev', hardware_tested: false,
  source_commit: commit, source_tree: tree, models: { pro: pkg } }] };
const qualification = { schema: 1, installer_sha256: installerFingerprint(), packages: { [digest]: {
  source_commit: commit, source_tree: tree, size: raw.length, product, url: pkg.pkg,
  integration: { ref: 'nightly', commit, tree, evidence_url: evidence },
  controller: { strategy: 'owner-supplied', sha256: hash(ctl), size: 68 },
  budgets: { app_code_free: 5620, ram_free: 460, pool_free: 8324 },
  gates: Object.fromEntries(requiredGates.map(g => [g, { status: 'PASS', evidence_url: evidence }])),
  corresponding_source: `https://github.com/gobbajogger/X0X37-for-Loopy-Pro/archive/${commit}.tar.gz`,
  notices: `https://example.org/${digest}/notices.zip`,
} } };

validateCatalog(catalog,qualification);console.log('BASELINE_VALID');
const markers=['SYNTHETIC_PRIVATE_BINARY_BYTES_DO_NOT_PUBLISH','SYNTHETIC_PRIVATE_TOKEN','SYNTHETIC_PRIVATE_LIBRARY_LOCATOR','SYNTHETIC_PRIVATE_CTL_IMG'];
catalog.internal_controller_image_base64=markers[0];catalog.versions[0].models.pro.auth_token=markers[1];
qualification.internal_library_locator=markers[2];qualification.packages[digest].controller.vendor_payload=markers[3];
let admitted=true;try{validateCatalog(catalog,qualification)}catch{admitted=false}
const fs=await import('node:fs');fs.writeFileSync(process.argv[2],JSON.stringify({catalog,qualification,raw:[...raw],markers,admitted}));
