#!/usr/bin/env python3
"""Replay public allocator with exact controller config; no radio access."""
import os, re, shlex, subprocess, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
config=(root/'ble/ctl/src/config.c').read_text()
linker=(root/'ble/ctl/ctl.ld').read_text()
def val(name): return int(re.search(r'\b'+name+r'\s*=\s*(\d+)\s*;',config)[1])
hw=val('config_btctler_le_hw_nums')
assert hw in (1, 2), 'controller must have one or two hardware connections'
assert val('config_btctler_le_master_multilink')==(1 if hw == 2 else 0)
assert val('config_btctler_le_rx_nums')==5
assert val('config_btctler_le_acl_packet_length')==27
heap=int(re.search(r'CTL_HEAP_SIZE\s*=.*?:\s*(0x[0-9a-fA-F]+)\s*;',linker)[1],0)
credits=val('config_btctler_le_acl_total_nums')
with tempfile.TemporaryDirectory() as d:
 exe=Path(d)/'test'
 cmd=[os.getenv('CC','cc'),'-std=c11','-fno-pie','-no-pie','-O1','-g','-Wall','-Wextra','-Werror',f'-DTEST_CTL_HEAP={heap}',f'-DTEST_CTL_CREDITS={credits}',f'-DTEST_CTL_HW_NUMS={hw}',str(root/'tests/host/ble_controller_heap_test.c'),'-o',str(exe)]+shlex.split(os.getenv('SAN_FLAGS',''))
 subprocess.run(cmd,check=True); subprocess.run([str(exe)],check=True)

 # Handle 0 is a legal controller handle; the pair router must still route it.
 for handle in (0,1,64):
  subprocess.run([os.getenv('CC','cc'),'-std=c99','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',f'-DTEST_ACL_CREDITS={credits}',f'-DTEST_HANDLE_BASE={handle}','-I'+str(root/'firmware/src/ble'),str(root/'tests/host/ble_discovery_replay_test.c'),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
