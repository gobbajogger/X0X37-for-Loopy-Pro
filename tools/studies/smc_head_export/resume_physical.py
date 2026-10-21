#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Resume the ORIGINAL pending exporter with a reviewed host-only contract overlay.

Never stage a loader, migrate a receipt, change firmware, or relax generated
package validation. Reviewed code provenance and original source/OTA/receipt
identities must all match before opening MIDI. No keys/raw bytes are printed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import diagnostic_acquire_core as acquire
from diagnostic_acquire_core import bundle, fi
import smc_head_export_record as exporter

CONTRACT = 'native-key-preserved-head-v1'
# Exact transitive local host implementation; do not accept an arbitrary list.
FILES = (
    'tools/studies/smc_head_export/resume_physical.py',
    'tools/studies/smc_head_export/diagnostic_acquire_core.py',
    'tools/studies/smc_head_export/host.py',
    'tools/smc_vendor_bundle.py', 'tools/smc_vendor_install.py',
    'tools/smc_head_export_record.py', 'tools/fm1_install.py',
    'tools/fm1pkg_make.py', 'tools/add_ctl.py', 'tools/x0x_slots.py',
)
ROOT = Path(__file__).resolve().parents[3]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def verify_overlay(record, expected_sha):
    """Verify an independently reviewed code inventory, not a new DEV record."""
    raw = Path(record).read_bytes()
    bundle.need(digest(raw) == expected_sha, 'host overlay record identity mismatch')
    doc = json.loads(raw)
    bundle.need(set(doc) == {'schema', 'contract', 'scope', 'exporter', 'files', 'review'}, 'overlay record fields')
    bundle.need(doc['schema'] == 1 and doc['contract'] == CONTRACT and
                doc['scope'] == 'resume-original-head-export-only', 'overlay scope/contract')
    bundle.need(doc['exporter'] == {'source_sha256': exporter.SOURCE_SHA, 'ota_sha256': exporter.OTA_SHA},
                'overlay must retain original reviewed exporter')
    bundle.need(type(doc['files']) is dict and set(doc['files']) == set(FILES), 'exact overlay host inventory required')
    review = doc['review']
    bundle.need(type(review) is dict and set(review) == {'status', 'reviewer', 'evidence_sha256'} and
                review['status'] == 'independently-reviewed-host-only' and
                isinstance(review['reviewer'], str) and review['reviewer'].strip() and
                isinstance(review['evidence_sha256'], str) and len(review['evidence_sha256']) == 64 and
                all(c in '0123456789abcdef' for c in review['evidence_sha256']),
                'independent host-overlay review is required; proposal is not runnable')
    for name in FILES:
        bundle.need(digest((ROOT/name).read_bytes()) == doc['files'][name], 'overlay host source changed: '+name)
    modules = ((acquire, 'tools/studies/smc_head_export/diagnostic_acquire_core.py'),
               (sys.modules[acquire.capture.__module__], 'tools/studies/smc_head_export/host.py'),
               (bundle, 'tools/smc_vendor_bundle.py'), (acquire.install, 'tools/smc_vendor_install.py'),
               (exporter, 'tools/smc_head_export_record.py'), (fi, 'tools/fm1_install.py'),
               (bundle.pm, 'tools/fm1pkg_make.py'), (bundle.add_ctl, 'tools/add_ctl.py'),
               (fi.xs, 'tools/x0x_slots.py'))
    for module, name in modules:
        bundle.need(Path(module.__file__).resolve() == ROOT/name, 'mixed overlay module origin: '+name)
    return doc


def original_transaction(raw, source_sha, ota_sha, output, receipt):
    """No replacement source, fresh receipt, or copied/relocated output identity."""
    bundle.need(source_sha == exporter.SOURCE_SHA and ota_sha == exporter.OTA_SHA,
                'only original reviewed exporter hashes are allowed')
    acquire.source(raw, source_sha, ota_sha)
    path = Path(receipt)
    bundle.need(path.is_file(), 'original saved exporter receipt required; no new transaction')
    expected = dict(schema=1, profile='elite-head-export-dev', source_sha256=source_sha,
                    ota_sha256=ota_sha, output=str(Path(output).resolve()))
    bundle.need(json.loads(path.read_text()) == expected, 'original receipt/source/output identity mismatch')


def loader_only(*_):
    raise ValueError('original pending exporter loader required; new staging is forbidden')


def run(backend, raw, source_sha, ota_sha, output, receipt, *, overlay_record,
        overlay_sha256, acknowledge, port=None, protected_paths=(), emit=print):
    verify_overlay(overlay_record, overlay_sha256)
    original_transaction(raw, source_sha, ota_sha, output, receipt)
    # The original stage identity/handshake path is retained. Its injectable
    # validator runs only after source/owned-record/nonce/transport checks.
    def validate(head):
        return bundle.physical_head_key(head, diagnostics=lambda row: emit(json.dumps(row, sort_keys=True)))
    return acquire.run(backend, raw, source_sha, ota_sha, output, receipt,
                       acknowledge=acknowledge, cold_check=loader_only, port=port,
                       protected_paths=[overlay_record, *protected_paths], validate_head=validate,
                       validation_provenance=dict(contract=CONTRACT, overlay_record_sha256=overlay_sha256,
                                                  source_sha256=source_sha, ota_sha256=ota_sha))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--resume-original-physical-head', action='store_true', required=True)
    for name in ('source', 'source-sha256', 'ota-sha256', 'output', 'receipt', 'overlay-record', 'overlay-sha256'):
        ap.add_argument('--'+name, required=True)
    ap.add_argument('--port')
    a = ap.parse_args()
    raw = Path(a.source).read_bytes()
    verify_overlay(a.overlay_record, a.overlay_sha256)
    original_transaction(raw, a.source_sha256, a.ota_sha256, a.output, a.receipt)
    if fi.foreground_guard() or fi.hardware_lock():
        return 10
    def ack():
        return fi.ask_tty('Resume ONLY if the original pending exporter is currently detected? No new staging or further power cycle during this attempt. '
                          'Disconnect amplified outputs/headphones. Preserve the original receipt and output paths. '
                          'The key blob is CRC-checked; unused payload CRC mismatches are reported, not repaired. [y/N] ')
    try:
        result = run(fi.MidoBackend(), raw, a.source_sha256, a.ota_sha256, a.output, a.receipt,
                     overlay_record=a.overlay_record, overlay_sha256=a.overlay_sha256,
                     acknowledge=ack, port=a.port, protected_paths=[a.source])
    except (ValueError, OSError, TimeoutError) as e:
        print('Resume stopped: '+str(e)+'. No fallback, staging or forced cleanup.', file=sys.stderr)
        return 2
    print('Complete head saved locally; cleanup '+('confirmed.' if result['cleanup'] == 'confirmed' else 'uncertain; retain original transaction.'))
    return 0 if result['cleanup'] == 'confirmed' else 4


if __name__ == '__main__':
    sys.exit(main())
