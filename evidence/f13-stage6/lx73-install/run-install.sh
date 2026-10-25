#!/bin/bash
set -u
cd '/Users/mac/Documents/x0x37/x0x37-f13-stage6-lx73'
export STY=1.x0x-install
/usr/bin/python3 tools/fm1_install.py '/Users/mac/Library/Application Support/x0x37/unattended-install/packets/9c1168cf83d298e0/x0x-smk37-elite.fwsc' --yes --commit 092589b84fd455355216cb0de93c4782236f7524 --dev-record '/Users/mac/Documents/x0x37/x0x37-f13-stage6-lx73/evidence/f13-stage5/lx73-devcheck/elite/record.json' --config dev --ctl '/Users/mac/Library/Application Support/x0x37/unattended-install/packets/956e3e214658aeaa/ctl.img' --backup '/Users/mac/Library/Application Support/x0x37/unattended-install/records/4cd9e2e9a47b63b1/slots-before-install.x0xslots' --port 'X0X FM-1' > '/Users/mac/Library/Application Support/x0x37/unattended-install/records/4cd9e2e9a47b63b1/install.log' 2>&1
echo __EXIT__:$? >> '/Users/mac/Library/Application Support/x0x37/unattended-install/records/4cd9e2e9a47b63b1/install.log'
