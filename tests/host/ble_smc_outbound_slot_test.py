#!/usr/bin/env python3
"""Compile the actual SMC BLE output function with deterministic host stubs.

Extracts the one production function without copying or rewriting its body.
No RF/device behavior is claimed.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
app = (ROOT / "firmware/src/ble.c").read_text()
functions = re.findall(
    r"^int plat_ble_smc_send_slot\s*\([^)]*\)\s*\{\n.*?^\}",
    app, flags=re.M | re.S
)
assert len(functions) == 1, f"expected one production slot sender, got {len(functions)}"
source = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef struct { unsigned slot; } blec_t;
static struct { struct { blec_t c, c2; } host; } ble_r = {{{0}, {1}}};
static int ble_central_role=1, ble_up=1, ble_host_on=1, ble_want=1, role=1;
static unsigned sends[2], locks, unlocks;
static int smc_ble_role_get(void) { return role; }
static uint32_t irq_save(void) { locks++; return 77; }
static void irq_restore(uint32_t x) { assert(x == 77); unlocks++; }
static int blec_midi_send(blec_t *c, const uint8_t *p, uint8_t n) {
    assert(p && n==3 && c->slot < 2);
    sends[c->slot]++;
    return 1;
}
''' + functions[0] + r'''
int main(void) {
    const uint8_t msg[3]={0x90,60,127};
    assert(plat_ble_smc_send_slot(0,msg,3));
    assert(plat_ble_smc_send_slot(1,msg,3));
    assert(sends[0]==1 && sends[1]==1);
    assert(!plat_ble_smc_send_slot(2,msg,3));
    assert(!plat_ble_smc_send_slot(255,msg,3));
    assert(locks==2 && unlocks==2); /* invalid slots must not take IRQ lock */
    ble_central_role=0;
    assert(!plat_ble_smc_send_slot(1,msg,3));ble_central_role=1;
    role=0;assert(!plat_ble_smc_send_slot(1,msg,3));role=1;
    ble_up=0;assert(!plat_ble_smc_send_slot(1,msg,3));ble_up=1;
    ble_host_on=0;assert(!plat_ble_smc_send_slot(1,msg,3));ble_host_on=1;
    ble_want=0;assert(!plat_ble_smc_send_slot(1,msg,3));ble_want=1;
    assert(sends[0]==1 && sends[1]==1 && locks==unlocks);
    printf("SMC outbound slot routing: two distinct peers, bounds and five off gates PASS\n");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="smc-ble-slots-") as td:
    src = Path(td) / "test.c"
    exe = Path(td) / "test"
    src.write_text(source)
    cc = shlex.split(os.environ.get("CC", "cc"))
    subprocess.run([*cc, "-std=c99", "-O2", "-Wall", "-Wextra", "-Werror",
                    str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
