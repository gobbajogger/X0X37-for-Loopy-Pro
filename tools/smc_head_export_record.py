#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fixed reviewed exporter artifact and recipe identity, not arbitrary CLI hashes."""
from pathlib import Path
import hashlib
import smc_vendor_bundle as b
SOURCE_SHA='dcea0622e2c26f45cf6ef21b8ca361b929ef1149e1acd1dacc245bc75dd88718'
OTA_SHA='e19ea1f7ebf3f00ac45b600f04c9a2a55159307b027ae06222d82d895f4784b4'
INPUTS = {'tools/studies/smc_bundle_loader/loader_quiesce.h': 'defd80d140ba8e55ad87fc1b86b908cc5d6918c750c993564a13cf3f40f59e0d',
 'tools/studies/smc_bundle_loader/record_owner.h': '4dc932e612a3a55525c27f97d9e160d1d19ad9b8686a313fd62686bea0a227cf',
 'tools/studies/smc_head_export/build_source.py': 'b97853ceb886368ed15978ef26a3a23c6db602a5eb34a3d4422f5c67a399b8cb',
 'tools/studies/smc_head_export/head_export.h': '87d015f7126d7da1e8f477868fd017e795ba99fd431d813c0914f5cd71510b07',
 'tools/studies/smc_head_export/loader_core.h': '4b8b5ac1de260f7151e66755391d97258148bf1cf88ad96237400faec996334e',
 'tools/studies/smc_head_export/prepare.py': '433b264d00b9fcbdecb760c29c6bb008533e3eddf6da80e22b14f570796bbf98'}

def validate(repo,path):
    for name,digest in INPUTS.items():
        b.need(hashlib.sha256((Path(repo)/name).read_bytes()).hexdigest()==digest,'retained exporter source changed: '+name)
    raw=Path(path).read_bytes()
    b.need(b.sha(raw)==SOURCE_SHA,'not the reviewed read-only exporter package')
    _,ota=b.unpack(raw)
    b.need(b.sha(ota)==OTA_SHA and len(ota)==8228 and b'X0X-HEAD-EXPORT\0' in ota[0x48:0x88],'exporter loader identity changed')
    return dict(profile='elite-head-export-dev',model=b.MODEL,source_sha256=SOURCE_SHA,
                ota_sha256=OTA_SHA,ota_size=len(ota),hardware_tested=False,
                scope='read-only head; owned staging/record writes only',source_inputs=INPUTS)
