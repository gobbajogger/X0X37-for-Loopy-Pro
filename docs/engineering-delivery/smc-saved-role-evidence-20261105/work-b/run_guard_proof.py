#!/usr/bin/env python3
"""Scratch proof of exact project_save control flow with deterministic stubs.

This does not implement real storage or a real stream layout. Production project.c
and the independently delivered saved-role fixture are never changed.
"""
from pathlib import Path
import base64
import hashlib
import json
import platform
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / 'project.c'
blob = SOURCE.read_bytes()
source_hash = hashlib.sha256(blob).hexdigest()
git_blob = hashlib.sha1(b'blob ' + str(len(blob)).encode() + b'\0' + blob).hexdigest()
assert git_blob == '82cc9eba89884e4310b3273941fb9b6335611415'
text = blob.decode()
matches = list(re.finditer(r'^int project_save\([^\n]*\)\s*\{\n.*?^\}', text, re.M | re.S))
assert len(matches) == 1
actual = matches[0][0]
(ROOT / 'project_save.extracted.c').write_text(actual)
macro = re.findall(r'^#define PROJ_STREAM [^\n]*', text, re.M)
assert len(macro) == 1
prefix = r'''/* SCRATCH HOST HARNESS: function below is extracted unmodified.
 * Types and all external calls are deterministic counted stubs.
 * Device buffer guard and engine-specific branches are disabled/uncovered.
 */
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define X0X_HOST 1
#define X0X_BASS_TAGGED_SOUND 0
#define X0X_BASS_PRESET_PREVIEW 0
#if X0X_HOST != 1 || X0X_BASS_TAGGED_SOUND != 0 || X0X_BASS_PRESET_PREVIEW != 0
#error unexpected host preprocessor configuration
#endif
#define OBJ_SET 0
#define OBJ_SOUND 1
'''+macro[0]+r'''
typedef struct { int set; int sound; } project_t; /* harness-only layout */
static project_t proj;
static uint8_t project_incompatible, settings_read_failed, settings_load_failed;
static uint32_t packed_len;
static int layout_valid, measure_rc, set_rc, sound_rc, stream_rc;
static unsigned layout_calls, capture_calls, measure_calls, save_calls, stream_calls;
static unsigned failures, checks, cases;
#define CHECK(c) do { ++checks; if (!(c)) { \
    ++failures; fprintf(stderr, "FAIL line=%d: %s\n", __LINE__, #c); \
} } while (0)
static int stream_layout_ok(const project_t *p, uint32_t format) {
    ++layout_calls;
    CHECK(p == &proj && format == PROJ_STREAM);
    return layout_valid;
}
static void capture_settings(int *p) {
    ++capture_calls; CHECK(p == &proj.set); *p=17;
}
static int measure_project(const project_t *p, uint32_t *len) {
    ++measure_calls; CHECK(p == &proj && len == &packed_len);
    *len=99; return measure_rc;
}
static int save_obj(unsigned obj, const void *p, size_t n) {
    ++save_calls;
    CHECK((obj == OBJ_SET && p == &proj.set && n == sizeof proj.set && save_calls == 1)
          || (obj == OBJ_SOUND && p == &proj.sound && n == sizeof proj.sound && save_calls == 2));
    return obj == OBJ_SET ? set_rc : sound_rc;
}
static int save_stream(void) { ++stream_calls; return stream_rc; }
'''
suffix = r'''
static void reset(unsigned incompatible, int valid, unsigned read_fence, unsigned load_fence) {
    memset(&proj, 0, sizeof proj);
    project_incompatible=(uint8_t)incompatible;
    settings_read_failed=(uint8_t)read_fence;
    settings_load_failed=(uint8_t)load_fence;
    layout_valid=valid;
    measure_rc=set_rc=sound_rc=stream_rc=0;
    layout_calls=capture_calls=measure_calls=save_calls=stream_calls=0;
    packed_len=123;
}
int main(void) {
    puts("CONFIG X0X_HOST=1 X0X_BASS_TAGGED_SOUND=0 X0X_BASS_PRESET_PREVIEW=0");
    puts("SCOPE actual_project_save_control_flow_with_stubs; device/engine branches NOT_RUN");
    for (unsigned incompatible=0; incompatible<=2; ++incompatible)
        for (int valid=0; valid<=1; ++valid)
            for (unsigned read_fence=0; read_fence<=1; ++read_fence)
                for (unsigned load_fence=0; load_fence<=1; ++load_fence) {
                    reset(incompatible, valid, read_fence, load_fence);
                    int expected=(incompatible || !valid) ? -9 :
                                 (read_fence || load_fence) ? -8 : 0;
                    int rc=project_save();
                    ++cases;
                    printf("CASE incompatible=%u valid=%d read=%u load=%u expected=%d actual=%d layout=%u capture=%u measure=%u save=%u stream=%u\n",
                           incompatible, valid, read_fence, load_fence, expected, rc,
                           layout_calls, capture_calls, measure_calls, save_calls, stream_calls);
                    CHECK(rc == expected);
                    CHECK(layout_calls == (incompatible ? 0u : 1u));
                    if (expected) {
                        CHECK(capture_calls == 0);
                        CHECK(measure_calls == 0);
                        CHECK(save_calls == 0);
                        CHECK(stream_calls == 0);
                        CHECK(packed_len == 123 && proj.set == 0);
                    } else {
                        CHECK(capture_calls == 1);
                        CHECK(measure_calls == 1);
                        CHECK(save_calls == 2);
                        CHECK(stream_calls == 1);
                        CHECK(packed_len == 99 && proj.set == 17);
                    }
                }
    /* Positive path and deterministic post-guard failure controls demonstrate
       the exact function actually reaches and evaluates its real call sites. */
    reset(0,1,0,0); measure_rc=1;
    CHECK(project_save() == -7);
    CHECK(capture_calls == 1 && measure_calls == 1 && save_calls == 0 && stream_calls == 0);
    reset(0,1,0,0); set_rc=1;
    CHECK(project_save() == -1);
    CHECK(capture_calls == 1 && measure_calls == 1 && save_calls == 1 && stream_calls == 0);
    reset(0,1,0,0); sound_rc=1;
    CHECK(project_save() == -2);
    CHECK(capture_calls == 1 && measure_calls == 1 && save_calls == 2 && stream_calls == 0);
    reset(0,1,0,0); stream_rc=7;
    CHECK(project_save() == 7);
    CHECK(capture_calls == 1 && measure_calls == 1 && save_calls == 2 && stream_calls == 1);
    printf("cases=%u checks=%u failures=%u result=%s\n", cases, checks, failures, failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
'''

guard = '    if (project_incompatible || !stream_layout_ok(&proj, PROJ_STREAM))\n        return -9; /* not one settings/sound/pattern object may be rewritten */\n'
fence = '    if (settings_read_failed || settings_load_failed) return -8; /* retry load before saving fallback defaults */\n'
assert actual.count(guard) == actual.count(fence) == 1
variants = {
    'actual': actual,
    'omit_incompatible_layout_guard': actual.replace(guard, '    (void)stream_layout_ok(&proj, PROJ_STREAM); /* scratch mutant: omit guard */\n'),
    'reorder_settings_fence_first': actual.replace(guard+fence, fence+guard),
    'omit_settings_fence': actual.replace(fence, '    (void)settings_read_failed; (void)settings_load_failed; /* scratch mutant: omit fence */\n'),
}
records=[]
def run(name, argv):
    p=subprocess.run(argv, cwd=ROOT, capture_output=True)
    (ROOT/(name+'.stdout')).write_bytes(p.stdout)
    (ROOT/(name+'.stderr')).write_bytes(p.stderr)
    records.append({'name':name,'argv':argv,'exit':p.returncode,
                    'stdout':name+'.stdout','stderr':name+'.stderr'})
    return p

environment = {'python':sys.version,'platform':platform.platform(),'machine':platform.machine()}
run('compiler_version',['cc','--version'])
run('compiler_target',['cc','-dumpmachine'])
run('uname',['uname','-a'])
for name, function in variants.items():
    harness=prefix+function+'\n'+suffix
    path=ROOT/(name+'.c');path.write_text(harness)
    if name == 'actual':
        assert function == actual
        prep=run('preprocess',['cc','-std=c99','-E','-P',str(path)])
        assert prep.returncode == 0
        preprocessed=prep.stdout.decode()
        assert 'store_stage_held' not in preprocessed
        assert 'engine_bass_sound_validate' not in preprocessed
        assert 'engine_bass_preview_changed' not in preprocessed
        m=run('preprocessor_macros',['cc','-std=c99','-dM','-E',str(path)])
        assert m.returncode == 0
        macros=dict(re.findall(r'^#define (X0X_\w+) (\d+)$',m.stdout.decode(),re.M))
        assert macros == {'X0X_HOST':'1','X0X_BASS_TAGGED_SOUND':'0','X0X_BASS_PRESET_PREVIEW':'0'}
        environment['host_macros']=macros
    modes = ('plain','ubsan') if name=='actual' else ('ubsan',)
    for mode in modes:
        exe=ROOT/(name+'-'+mode)
        argv=['cc','-std=c99','-O1','-g','-Wall','-Wextra','-Werror']
        if mode=='ubsan':argv+=['-fsanitize=undefined','-fno-sanitize-recover=all']
        argv += [str(path),'-o',str(exe)]
        compiled=run(name+'-'+mode+'-compile',argv)
        assert compiled.returncode == 0, compiled.stderr.decode()
        linked=run(name+'-'+mode+'-ldd',['ldd',str(exe)])
        if mode=='ubsan': assert b'libubsan.so.1' in linked.stdout
        executed=run(name+'-'+mode+'-execute',[str(exe)])
        expected_exit=0 if name=='actual' else 1
        assert executed.returncode==expected_exit
        if name=='actual':
            assert not executed.stderr
            assert b'cases=24' in executed.stdout and b'failures=0 result=PASS' in executed.stdout
        else:
            assert b'FAIL line=' in executed.stderr

assert SOURCE.read_bytes()==blob
retrieved=json.loads((ROOT/'project-response.json').read_text())['structuredContent']
assert base64.b64decode(retrieved['content'])==blob
summary={
    'scope':'actual project_save control-flow-with-stubs proof; NOT complete storage/flash/firmware qualification',
    'source':{'repository':'gobbajogger/X0X37-for-Loopy-Pro','commit':'8d43c53f6fc9d6aba749893c82d82a6e7409773a',
              'path':'firmware/src/app/project.c','bytes':len(blob),'blob_sha1':git_blob,'sha256':source_hash},
    'extracted_function':{'path':'project_save.extracted.c','sha256':hashlib.sha256(actual.encode()).hexdigest(),'bytes':len(actual.encode())},
    'environment':environment,
    'commands':records,
    'results':{'strict_plain':'PASS','strict_ubsan':'PASS','matrix_cases_per_run':24,
               'negative_controls':{k:'KILLED' for k in variants if k!='actual'},
               'preprocessor_configuration':'PASS','source_unchanged':'PASS',
               'engine_specific_branches':'NOT_RUN','device_store_stage_held_branch':'NOT_RUN',
               'full_storage_flash_firmware_qualification':'NOT_RUN'},
    'source_readback':{'bytes_equal_to_initial_and_GitHub_payload':True,'sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest()},
    'release':{'scratch_scope':'RELEASED','all_subprocesses_awaited':True,'background_processes_started':False,'tracked_source_or_tests_modified':False},
}
(ROOT/'worker-b.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary['results'],indent=2))
print((ROOT/'actual-plain-execute.stdout').read_text().splitlines()[-1])
print((ROOT/'actual-ubsan-execute.stdout').read_text().splitlines()[-1])
