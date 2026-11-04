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

# Extract complete production control flow, including both failure labels. The
# settings-read fence is set by the actual payload reader, not a synthetic flag.
def function(name):
    match = unique(r'^(?:[^\n]* )?' + name + r'\([^;\n]*\)[^\n{};]*\s*\{\n.*?^\}',
                   PROJECT, name)
    return match.group(0)

loader = function('read_project')
assert 'uint8_t device_bt = proj.set.bt;' in loader
assert '\nunsupported:' in loader and '\nfailed:' in loader
settings_reader = function('read_settings_payload') + '\n' + function('load_settings')
load_wrapper = function('project_load')
default_wrapper = unique(r'^void project_defaults\(void\) \{[^\n]+\}',
                         PROJECT, 'boot fallback defaults').group(0)
boot_fallback = unique(r'    if \(project_load\(\) != 0\)\n        project_defaults\(\);[^\n]*',
                       MAIN, 'boot load fallback').group(0)
save_fence = unique(r'^    if \(settings_read_failed \|\| settings_load_failed\) return -8;[^\n]*',
                    PROJECT, 'settings save fence').group(0)
settings_type = unique(r'^typedef struct \{\n    uint32_t magic, format;.*?^\} settings_t;',
                       HEADER, 'settings layout').group(0)
format_macros = HEADER[HEADER.index('#define PROJ_MAGIC '):HEADER.index('\n#endif', HEADER.index('#define PROJ_MAGIC ')) + len('\n#endif')]
stream_context = PROJECT[PROJECT.index('#define PROJ_STREAM '):PROJECT.index('\n#define STREAM_CAP ')]
byte_helpers = function('hash_of') + '\n' + function('get32') + '\n' + function('put32')
# Preserve the real bitmasks and exact default assignment.
bt_macros = '\n'.join(unique(r'^#define\s+' + key + r'\s+[^\n]+$', HEADER, key).group(0)
                     for key in ('PROJ_BT_CLK_IN', 'PROJ_BT_SMC_CENTRAL', 'PROJ_BT_SMC_USER', 'PROJ_DEVICE_BT'))
default_bt = unique(r'^\s*s->bt\s*=\s*\(uint8_t\)\([^\n]*\);', PROJECT, 'production default Bluetooth assignment').group(0)

c_source = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define X0X_BLE 1
#define BOARD_SMK37 1
#define X0X_ORIGINAL_SYNTH 1
#define X0X_BASS_TAGGED_SOUND 0
/* Object numbers and the project sound below are harness-only storage stubs. */
#define OBJ_SET 0
#define OBJ_PAT0 1
#define OBJ_PAT1 2
#define OBJ_SONG 3
#define OBJ_MOTION 4
#define OBJ_NOBJ 5
''' + format_macros + '\n' + bt_macros + '\n' + settings_type + '\n' + stream_context + r'''
typedef struct { settings_t set; int sound; } project_t;
static project_t proj;
static uint8_t settings_read_failed, settings_load_failed, project_incompatible;
static uint8_t stream_previous, stored_known[OBJ_NOBJ], io_buf[256];
static uint64_t stored_hash[OBJ_NOBJ];
static uint32_t packed_len;
static int settings_result, sound_result, stream_result, validate_result, checks;
static unsigned sound_reads, stream_reads, validations;
static settings_t saved;
''' + byte_helpers + r'''
static int plat_store_load(unsigned obj, void *out, unsigned capacity) {
    assert(obj == OBJ_SET && out == io_buf && capacity >= sizeof saved + 4u);
    if (settings_result) return settings_result;
    put32(io_buf, PROJ_FORMAT);
    memcpy(io_buf + 4u, &saved, sizeof saved);
    return (int)sizeof saved + 4;
}
''' + settings_reader + r'''
static int load_sound(int *out) {
    sound_reads++;
    *out = 0;
    return sound_result;
}
static int load_stream_object(int k) {
    assert(k >= 0 && k < NSTREAM);
    stream_reads++;
    put32(io_buf, PROJ_STREAM);
    return stream_result;
}
static int load_stream(project_t *pr, int n, uint32_t format) {
    assert(pr == &proj && n >= 4 && format == PROJ_STREAM);
    return 0;
}
static int project_validate_for(const project_t *pr) {
    assert(pr == &proj);
    validations++;
    return validate_result;
}
static void patterns_defaults(project_t *pr) { assert(pr == &proj); }
static void arrange_defaults(project_t *pr) { assert(pr == &proj); }
static void project_defaults_for(project_t *pr, uint8_t device_bt) {
    memset(pr, 0, sizeof *pr);
    settings_t *s = &pr->set;
    pr->set.magic=PROJ_MAGIC;
    pr->set.format=PROJ_FORMAT;
    pr->set.snd_rev=PROJ_SND_REV;
    pr->set.stereo=PROJ_STEREO;
''' + default_bt + r'''
}
''' + default_wrapper + '\n' + loader + '\n' + load_wrapper + r'''
/* Only the real settings fence is exercised; this is not a full save test. */
static int settings_save_gate(void) {
''' + save_fence + r'''
    return 0;
}
static void simulate_boot(void) {
''' + boot_fallback + r'''
}
static unsigned n_events, prepare_arg, on_arg;
static char events[8];
static void ble_prepare(unsigned on) { events[n_events++] = 'P';prepare_arg=on; }
static void ble_start(void) { events[n_events++] = 'S'; }
static void plat_ble_set(unsigned on) { events[n_events++] = 'T';on_arg=on; }
static void bluetooth_boot(void) {
''' + boot_body + r'''
}
#define CHECK(cond) do { checks++; if (!(cond)) { \
    fprintf(stderr, "FAIL at line %d: %s\n", __LINE__, #cond); return 1; } \
} while (0)
static void setup(unsigned initial, int result, unsigned stored) {
    memset(&proj,0,sizeof proj);
    proj.set.bt = (uint8_t)initial;
    settings_result=result;
    sound_result=stream_result=-1; validate_result=0;
    settings_read_failed=settings_load_failed=project_incompatible=0;
    sound_reads=stream_reads=validations=0;
    packed_len=99; stream_previous=3;
    memset(stored_known, 1, sizeof stored_known);
    memset(&saved, 0, sizeof saved);
    saved.magic=PROJ_MAGIC; saved.format=PROJ_FORMAT;
    saved.snd_rev=PROJ_SND_REV; saved.stereo=PROJ_STEREO;
    saved.bt=(uint8_t)stored;
}
static int check_boot(unsigned expected) {
    CHECK(proj.set.bt == (unsigned)(expected | PROJ_BT_CLK_IN));
    n_events=prepare_arg=on_arg=0; memset(events,0,sizeof events);
    bluetooth_boot();
    CHECK(n_events == 3u && !memcmp(events, "PST", 3u));
    CHECK(prepare_arg == 1u); /* controller initializes despite saved OFF */
    CHECK(on_arg == (expected & 1u)); /* but saved OFF stays radio-off */
    return 0;
}
static int scenario(unsigned initial, int result, unsigned stored, unsigned expected) {
    setup(initial, result, stored);
    simulate_boot();
    return check_boot(expected);
}
static int read_failure(int rc, unsigned initial) {
    setup(initial, rc, 1u | PROJ_BT_SMC_CENTRAL);
    CHECK(project_load() == rc);
    CHECK(settings_read_failed == 1 && settings_load_failed == 1);
    CHECK(settings_save_gate() == -8);
    CHECK(sound_reads == 0 && stream_reads == 0 && validations == 0);
    CHECK(stored_known[OBJ_SET] == 0);
    CHECK(packed_len == 0 && stream_previous == 0);
    CHECK(project_incompatible == 0);
    project_defaults(); /* the caller's boot fallback must preserve the role */
    return check_boot(initial);
}
static int later_failure(unsigned stored, int sound_rc, int stream_rc, int validation_rc,
                         unsigned damaged) {
    setup(PROJ_BT_SMC_USER, 0, stored | PROJ_BT_CLK_IN);
    sound_result=sound_rc; stream_result=stream_rc; validate_result=validation_rc;
    CHECK(project_load() == -1);
    CHECK(settings_read_failed == 0 && settings_load_failed == 1);
    CHECK(settings_save_gate() == -8);
    CHECK(sound_reads == 1 && stream_reads > 0);
    CHECK(project_incompatible == damaged);
    CHECK(packed_len == 0 && stream_previous == 0);
    project_defaults();
    return check_boot(stored);
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
    /* Read failures fence saves and bypass the absent-only default branch. */
    if (read_failure(-2, 0) || read_failure(-8, 0)) return 1;
    if (read_failure(-8, PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER)) return 1;
    /* A selected DEVICE/ON or USER role also prevents first-boot central. */
    if (scenario(1u, -1, 0, 1u)) return 1;
    if (scenario(PROJ_BT_SMC_USER, -1, 0, PROJ_BT_SMC_USER)) return 1;
    /* With no saved settings, a later transient read alone must fence saves. */
    setup(0, -1, 0);
    sound_result=-8;
    CHECK(project_load() == -1);
    CHECK(settings_read_failed == 0 && settings_load_failed == 1);
    CHECK(settings_save_gate() == -8);
    CHECK(project_incompatible == 0);
    project_defaults();
    if (check_boot(1u | PROJ_BT_SMC_CENTRAL)) return 1;
    /* Saved roles survive real sound, stream and validation failure paths. */
    const unsigned roles[] = {0, 1u, PROJ_BT_SMC_CENTRAL, PROJ_BT_SMC_CENTRAL | 1u};
    for (unsigned k=0; k<sizeof roles/sizeof roles[0]; ++k) {
        if (later_failure(roles[k], -1, (int)STREAM_HDR, 0, 2)) return 1;
        if (later_failure(roles[k], 0, -1, 0, 2)) return 1;
        if (later_failure(roles[k], -8, (int)STREAM_HDR, 0, 0)) return 1;
        if (later_failure(roles[k], 0, -2, 0, 0)) return 1;
        if (later_failure(roles[k], 0, (int)STREAM_HDR, -1, 2)) return 1;
    }
    /* A readable incompatible payload cannot adopt its unvalidated role. */
    setup(PROJ_BT_SMC_USER, 0, 1u | PROJ_BT_SMC_CENTRAL);
    saved.magic=0;
    CHECK(project_load() == -9);
    CHECK(settings_read_failed == 0 && settings_load_failed == 1);
    CHECK(project_incompatible == 1);
    CHECK(sound_reads == 0 && stream_reads == 0);
    CHECK(settings_save_gate() == -8);
    project_defaults();
    if (check_boot(PROJ_BT_SMC_USER)) return 1;
    /* A read -9 and a readable incompatible object are different paths. */
    setup(0, -9, 0);
    CHECK(project_load() == -9);
    CHECK(settings_read_failed == 1 && settings_load_failed == 1);
    CHECK(project_incompatible == 1);
    CHECK(sound_reads == 0 && stream_reads == 0);
    /* A Bluetooth read retry alone must not clear a full-load fence. */
    settings_result=0;
    CHECK(load_settings(&saved) == 0);
    CHECK(settings_read_failed == 0 && settings_load_failed == 1);
    CHECK(settings_save_gate() == -8);
    /* Only a later successful project load clears the load fence. */
    sound_result=0; stream_result=(int)STREAM_HDR;
    saved.bt=PROJ_BT_CLK_IN | PROJ_BT_SMC_CENTRAL;
    CHECK(project_load() == 0);
    CHECK(settings_read_failed == 0 && settings_load_failed == 0);
    CHECK(project_incompatible == 0);
    CHECK(settings_save_gate() == 0);
    if (check_boot(PROJ_BT_SMC_CENTRAL)) return 1;
    puts("SMC boot/persistence production C slices: PASS");
    printf("checks=%d\n", checks);
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
        print('+', shlex.join(cmd), flush=True)
        subprocess.run(cmd, check=True)
        print('+', str(exe), flush=True)
        subprocess.run([str(exe)], check=True)
print('Production-source extraction and host regression: PASS')
