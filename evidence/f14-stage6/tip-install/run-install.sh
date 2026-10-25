#!/bin/bash
set -u
cd '/Users/mac/Documents/x0x37/x0x37-for-loopy-pro'
export X0X_DEV_PADCAL=1
/usr/bin/python3 tools/fm1_install.py '/Users/mac/Library/Application Support/x0x37/unattended-install/packets/4dc49bad6e8b555a/x0x-smk37-elite.fwsc' --yes   --commit b48cd8002a6d4d509010e04b8a8f1cfcbbdee64a   --dev-record '/Users/mac/Documents/x0x37/x0x37-for-loopy-pro/evidence/f14-tip-packages/elite/record.json'   --config dev   --ctl '/Users/mac/Library/Application Support/x0x37/unattended-install/packets/956e3e214658aeaa/ctl.img'   --backup '/Users/mac/Library/Application Support/x0x37/unattended-install/records/5f32e52e93056553/slots-before-install.x0xslots'   --port 'X0X FM-1'   > '/Users/mac/Library/Application Support/x0x37/unattended-install/records/5f32e52e93056553/install.log' 2>&1
echo __EXIT__:$? >> '/Users/mac/Library/Application Support/x0x37/unattended-install/records/5f32e52e93056553/install.log'
