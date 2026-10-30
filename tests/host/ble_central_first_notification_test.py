#!/usr/bin/env python3
"""Compile actual BLE central callback/link notification functions with native stubs.

A connected ATT batch can carry the subscribe response followed immediately by
BLE MIDI, before ble_central_service()'s final link-up notification. The first
new-session MIDI must not precede its link epoch. Mock transport only, NOT hardware.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BLE_SOURCE = Path(os.environ.get("BLE_SOURCE", ROOT / "firmware/src/ble.c"))
source = BLE_SOURCE.read_text(encoding="utf-8")


def extract(name):
    match = re.search(r"^static void " + re.escape(name) + r"\([^\n]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError("BLE production function missing: " + name)
    depth, end = 1, match.end()
    while depth:
        if end == len(source):
            raise RuntimeError("unclosed BLE function: " + name)
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


prefix = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static uint8_t ble_central_link_seen[2];
static int ble_want=1;
static int smc_ble_role_get(void) { return 1; }
typedef struct { int state; } blec_t;
static struct { struct { blec_t c,c2; } host; } ble_r;
static int blec_connected(const blec_t *c) { return c->state == 1; }
static unsigned count, link_events, midi_events, inlogs, failures, checks;
static char events[16];
static void smc_ble_link_slot(unsigned slot,int up) { (void)slot; events[count++] = up ? 'U' : 'D'; ++link_events; }
static void smc_ble_midi_slot(unsigned slot,const uint8_t *p, uint8_t n)
{ (void)slot; if (!p || !n) abort(); events[count++]='M'; ++midi_events; }
static void smk_inlog(uint8_t src, uint32_t value)
{ (void)src; (void)value; ++inlogs; }
static void ble_central_link_notify_slot(unsigned slot,int up);
"""
suffix = r"""
#define CHECK(cond) do { ++checks; if(!(cond)) { ++failures; \
    fprintf(stderr, "FAIL check %u: %s\\n", checks, #cond); } } while(0)
static void reset(void)
{ memset(ble_central_link_seen,0,sizeof ble_central_link_seen); ble_r.host.c.state=ble_r.host.c2.state=0;
  count=link_events=midi_events=inlogs=0; memset(events,0,sizeof events); }
int main(void)
{
    static const uint8_t note[3]={0x90u,8u,127u};
    static const uint8_t mmc[6]={0xF0u,0x7Fu,0x7Fu,6u,16u,0xF7u};
    for(unsigned slot=0;slot<2;slot++) for(unsigned offset=0; offset<4; ++offset) {
        reset(); (slot ? &ble_r.host.c2 : &ble_r.host.c)->state=1;
        /* BLEC_CONNECTED transition + first MIDI callback in one TIMER5 batch;
         * the service-level link notify has not been called yet. */
        ble_central_midi(note,3u,(void *)(uintptr_t)slot);
        ble_central_link_notify_slot(slot,1);
        CHECK(count==2u); CHECK(events[0]=='U'); CHECK(events[1]=='M');
        CHECK(link_events==1u); CHECK(midi_events==1u); CHECK(inlogs==1u);
        ble_central_midi(mmc,6u,(void *)(uintptr_t)slot);
        CHECK(count==3u); CHECK(events[2]=='M');
        CHECK(link_events==1u); CHECK(inlogs==2u);
        if(offset==0) {
            ble_central_link_notify_slot(slot,0);
            CHECK(link_events==2u); CHECK(events[3]=='D');
            ble_central_link_notify_slot(slot,1);
            CHECK(link_events==3u); CHECK(events[4]=='U');
            ble_central_midi(note,3u,(void *)(uintptr_t)slot);
            CHECK(link_events==3u); CHECK(events[5]=='M');
        }
    }
    reset();
    ble_central_midi(note,3u,NULL);
    CHECK(link_events==0u); CHECK(count==1u); CHECK(events[0]=='M');
    printf("BLE TRANSPORT CALLBACK CHECKS %u FAILURES %u\\n",checks,failures);
    return failures ? 1 : 0;
}
"""
c_source = (prefix + "\n" + extract("ble_central_midi") + "\n" +
            extract("ble_central_link_notify_slot") + "\n" + suffix)
with tempfile.TemporaryDirectory(prefix="ble-first-notification-") as tmp:
    code = Path(tmp) / "test.c"
    exe = Path(tmp) / "test"
    code.write_text(c_source, encoding="utf-8")
    cmd = [*shlex.split(os.environ.get("CC", "gcc")),
           "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
           *shlex.split(os.environ.get("SAN_FLAGS", "")),
           str(code), "-o", str(exe)]
    print("SOURCE", BLE_SOURCE, flush=True)
    print("COMMAND", " ".join(cmd), flush=True)
    subprocess.run(cmd, check=True)
    subprocess.run([str(exe)], check=True)
