#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Real Updater.find/wait_for/verify guards with mocked MIDI links; no hardware.

Run: python3 -B tests/host/ota_no_ambiguous_target_test.py
"""
import importlib.util
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
spec = importlib.util.spec_from_file_location('fm1_install_guard', ROOT / 'tools' / 'fm1_install.py')
f = importlib.util.module_from_spec(spec)
spec.loader.exec_module(f)
f.DELAY.update(start=0, loader=0, reboot=0, retry=.002, wait_reboot=.015, wait_loader=.015, idle_check=0)

class Link:
    def __init__(self, ident):
        self.identity = f.Identity(ident, ident.split('_')[0], 900)
        self.sent = []
        self.closed = False
    def send(self, data):
        self.sent.append(data)
        return True
    def close(self):
        self.closed = True

class Backend:
    def __init__(self, pairs):
        self.ports = dict(pairs)
        self.opened = []
    def input_names(self):
        return list(self.ports)
    def output_names(self):
        return list(self.ports)
    def open(self, port, output):
        assert port == output
        l = Link(self.ports[port])
        self.opened.append((port, l))
        return l

# Tests the exact Updater code, not a manually reimplemented search.
f.handshake = lambda link, tries=2: link.identity

class Updater(f.Updater):
    def serve(self, *args):
        return (1, True, False)
    def write(self, ota, image, step):
        self.wrote_to = ota.name
        ota.link.close()

passed = total = 0

def check(test, label):
    global passed,total
    total += 1
    passed += bool(test)
    print(('PASS' if test else 'FAIL') + ' ' + label)

def refuses(code, function):
    try:
        function()
    except f.InstallError as e:
        return e.code == code
    return False

P='SMK-37 Pro_900'
start='SMK-37 Pro_016'
port1='SMK-37 Pro USB A'
port2='SMK-37 Pro USB B'

# This preflight occurs before any backup or firmware OTA command.
b = Backend([(port1, start),(port2,start)])
u=Updater(b)
check(refuses('ambiguous', lambda:u.find(only_port=False)), 'refuse multiple running keyboard identities preflight')
check(all(link.closed for _,link in b.opened), 'close every opened link after ambiguity')

b=Backend([(port1,'ota-'+P),(port2,'ota-'+P)])
u=Updater(b)
check(refuses('ambiguous',lambda:u.wait_for(lambda i:i.loader, .05)), 'refuse multiple identical matching OTA loaders before flash')
check(not hasattr(u,'wrote_to'), 'ambiguous loader never selected for write')

b=Backend([(port1,P),(port2,P)])
u=Updater(b)
check(refuses('ambiguous', lambda:u.verify(P,lambda *args:None)), 'refuse two same-version devices for postflash verification')
check(refuses('ambiguous', lambda:u.wait_for(lambda i: not i.loader and i.text==P,.05)), 'refuse multiple matching sample-restore/controller-readback targets')

# Unrelated/nonmatching devices must not mask a valid target after install.
b=Backend([('FM-1 USB','FM-1_900'),(port1,'SMK-37 Elite_900'),(port2,P)])
u=Updater(b)
check(u.verify(P,lambda *args:None)==P, 'exact product passes despite foreign/stale sibling')

b=Backend([(port1,'SMK-37 Pro_899')])
u=Updater(b)
check(refuses('mismatch',lambda:u.verify(P,lambda *args:None)), 'stale-only product preserves mismatch after searching')

b=Backend([(port1,P)])
u=Updater(b)
check(u.verify(P,lambda *args:None)==P, 'lone exact target verifies normally')

b=Backend([(port1,'ota-'+P)])
u=Updater(b)
check(u.check(f.Device(Link(start),f.Identity(start,'SMK-37 Pro',16),port1),b'image',P,lambda *args:None).name==port1,
      'lone matching package loader still accepted')

b=Backend([(port1,'ota-SMK-37 Elite_900')])
u=Updater(b)
check(refuses('model',lambda:u.install(f.Device(Link('ota-SMK-37 Elite_900'),
    f.Identity('ota-SMK-37 Elite_900','ota-SMK-37 Elite',900),port1),b'image',P)),
    'wrong-model already-in-loader resume fails closed')

b=Backend([(port1,'ota-'+P),(port2,'ota-'+P)])
u=Updater(b)
check(refuses('ambiguous',lambda:u.check(f.Device(Link(start),f.Identity(start,'SMK-37 Pro',16),port1),b'image',P,lambda *args:None)),
      'duplicate actual package loader discovered by checked handover before write')

# Explicit --port may prioritize a valid device, but subsequent loader and
# post-install operations MUST detect another candidate rather than fall back.
b=Backend([(port1,'ota-'+P),(port2,'ota-'+P)])
u=Updater(b, port='USB A')
chosen = u.find(lambda i:i.loader)
check(chosen is not None and chosen.name==port1,
      'explicit port selects its requested keyboard on initial lookup')
if chosen: chosen.link.close()
check(refuses('ambiguous',lambda:u.wait_for(lambda i:i.loader,.05)),
      'reconnect ignores --port fallback priority and refuses duplicate eligible loaders')

print(f'CHECKS={passed}/{total} FAILURES={total-passed}')
sys.exit(total!=passed)
