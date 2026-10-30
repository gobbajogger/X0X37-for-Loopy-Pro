#!/usr/bin/env python3
"""Compile the production Bluetooth boot and saved-role decision slices.

The firmware is not executed. Only the relevant production C statements are
compiled with deterministic stubs, which makes this a scoped host test, not a
validated Elite/Pro build or device connection test.
"""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
MAIN = (ROOT / 'firmware/src/app/main_fm1.c').read_text(encoding='utf-8')
PROJECT = (ROOT / 'firmware/src/app/project.c').read_text(encoding='utf-8')
HEADER = (ROOT / 'firmware/src/app/x0x.h').read_text(encoding='utf-8')


def unique(pattern, text, label):
    matches = list(re.finditer(pattern, text, re.M | re.S))
    assert len(matches) == 1, f'{label}: expected once, found {len(matches)}'
    return matches[0]

# No hard-coded reconstructed replacement of these production statements.
boot_marker = unique(r'\s*engine_brk_loops\(\);\s*#if X0X_BLE\s*\n', MAIN, 'boot boundary')
boot_end = MAIN.index('\n#endif', boot_marker.end())
boot_body = MAIN[boot_marker.end():boot_end]
assert 'ble_prepare(' in boot_body and 'ble_start();' in boot_body
assert '#if' not in boot_body and '#endif' not in boot_body

# One revision-13 loader remains after the legacy-engine removal (18481a63).
original = unique(r'static int read_project\(project_t \*pr\)\s*\{\s*\n', PROJECT, 'revision-13 loader')
section = PROJECT[original.end():PROJECT.index('\nunsupported:', original.end())]
assert 'uint8_t device_bt = proj.set.bt;' in section
start = section.index('    int bad = 0, rc = load_settings(&set);')
end = section.index('    rc = load_sound(&pr->sound);', start)
settings_body = section[start:end]
assert 'goto unsupported;' in settings_body
# Preserve the real bitmasks and exact default assignment.
bt_macros = '\n'.join(unique(r'^#define\s+' + key + r'\s+[^\n]+$', HEADER, key).group(0)
                     for key in ('PROJ_BT_CLK_IN', 'PROJ_BT_SMC_CENTRAL', 'PROJ_BT_SMC_USER', 'PROJ_DEVICE_BT'))
default_bt = unique(r'^\s*pr->set\.bt\s*=\s*\(uint8_t\)\([^\n]*\);', PROJECT, 'production default Bluetooth assignment').group(0)

c_source = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define X0X_BLE 1
#define BOARD_SMK37 1
#define PROJ_MAGIC 0x1234u
#define PROJ_FORMAT 13u
#define PROJ_SND_REV 1u
#define PROJ_STEREO 1u
''' + bt_macros + r'''
typedef struct {
    uint32_t magic, format, snd_rev, stereo;
    uint8_t bt;
} settings_t;
typedef struct { settings_t set; } project_t;
static project_t proj;
static int settings_result, checks;
static settings_t saved;
static int load_settings(settings_t *out) {
    if (settings_result == 0) *out = saved;
    return settings_result;
}
static void project_defaults_for(project_t *pr, uint8_t device_bt) {
    memset(pr, 0, sizeof *pr);
    pr->set.magic=PROJ_MAGIC;
    pr->set.format=PROJ_FORMAT;
    pr->set.snd_rev=PROJ_SND_REV;
    pr->set.stereo=PROJ_STEREO;
''' + default_bt + r'''
}
static void project_defaults(void) { project_defaults_for(&proj, proj.set.bt); }
static int choose_saved_role(void) {
    project_t *pr = &proj;
    uint8_t device_bt = proj.set.bt;
    project_defaults_for(&proj, device_bt);
    settings_t set = proj.set;
''' + settings_body + r'''
    if (bad) project_defaults_for(&proj, device_bt);
    return bad ? -1 : 0;
unsupported:
    return -9;
}
static void simulate_boot(void) {
    if (choose_saved_role() != 0) project_defaults();
}
static unsigned n_events, prepare_arg, on_arg;
static char events[8];
static void ble_prepare(unsigned on) { events[n_events++] = 'P';prepare_arg=on; }
static void ble_start(void) { events[n_events++] = 'S'; }
static void __attribute__((unused)) plat_ble_set(unsigned on) { events[n_events++] = 'T';on_arg=on; }
static void bluetooth_boot(void) {
''' + boot_body + r'''
}
#define CHECK(cond) do { checks++; if (!(cond)) { \
    fprintf(stderr, "FAIL at line %d: %s\\n", __LINE__, #cond); return 1; } \
} while (0)
static int scenario(unsigned initial, int result, unsigned stored, unsigned expected) {
    memset(&proj,0,sizeof proj);
    proj.set.bt = (uint8_t)initial;
    settings_result=result;
    saved.magic=PROJ_MAGIC; saved.format=PROJ_FORMAT;
    saved.snd_rev=PROJ_SND_REV; saved.stereo=PROJ_STEREO;
    saved.bt=(uint8_t)stored;
    simulate_boot();
    CHECK(proj.set.bt == (unsigned)(expected | PROJ_BT_CLK_IN));
    n_events=prepare_arg=on_arg=0; memset(events,0,sizeof events);
    bluetooth_boot();
    CHECK(n_events == 3u && !memcmp(events, "PST", 3u));
    CHECK(prepare_arg == 1u); /* controller initializes despite saved OFF */
    CHECK(on_arg == (expected & 1u)); /* but saved OFF stays radio-off */
    return 0;
}
int main(void) {
    /* Brand-new keyboard: no settings -> start central and try SMC. */
    if (scenario(0, -1, 0, 1u | PROJ_BT_SMC_CENTRAL)) return 1;
    /* A fully stored role may be retained even when sound/project load later fails. */
    if (scenario(0, 0, PROJ_BT_CLK_IN | PROJ_BT_SMC_CENTRAL, PROJ_BT_SMC_CENTRAL)) return 1;
    if (scenario(0, 0, PROJ_BT_CLK_IN, 0u)) return 1; /* saved DEVICE/OFF */
    if (scenario(0, 0, PROJ_BT_CLK_IN | 1u, 1u)) return 1; /* saved DEVICE/ON */
    if (scenario(0, 0, PROJ_BT_CLK_IN | PROJ_BT_SMC_CENTRAL | 1u,
                 PROJ_BT_SMC_CENTRAL | 1u)) return 1; /* saved SMC/ON */
    if (scenario(PROJ_BT_SMC_CENTRAL, -1, 0, PROJ_BT_SMC_CENTRAL)) return 1;
    /* Storage read errors other than ABSENT do not silently auto-enable radio. */
    if (scenario(0, -2, 0, 0u)) return 1;
    puts("SMC boot/persistence production C slices: PASS");
    printf("checks=%d\\n", checks);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='smc-boot-contract-') as d:
    source = Path(d) / 'smc_boot.c'
    source.write_text(c_source)
    for flags in ([], ['-fsanitize=undefined', '-fno-sanitize-recover=all']):
        exe = Path(d) / ('smc_ubsan' if flags else 'smc_plain')
        cmd = [*shlex.split(os.environ.get('CC', 'cc')),
               '-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
               *flags, str(source), '-o', str(exe)]
        subprocess.run(cmd, check=True)
        subprocess.run([str(exe)], check=True)
print('Production-source extraction and host regression: PASS')
