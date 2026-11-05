/* SCRATCH HOST HARNESS: function below is extracted unmodified.
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
#define PROJ_STREAM 13u                 /* compact Chicago schema2; current raw patterns/arrangement */
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
int project_save(void)
{
#if X0X_BASS_TAGGED_SOUND
    if(engine_bass_sound_validate(&proj.sound))return -9;
    for(int v=0;v<NBASS;v++)if(engine_bass_preset_pending(v))return -8;
#elif X0X_BASS_PRESET_PREVIEW
    if(engine_bass_preview_changed()) return -9; /* no untagged preview writes */
#endif
    if (project_incompatible || !stream_layout_ok(&proj, PROJ_STREAM))
        return -9; /* not one settings/sound/pattern object may be rewritten */
    (void)settings_read_failed; (void)settings_load_failed; /* scratch mutant: omit fence */
#ifndef X0X_HOST
    extern int store_stage_held(void);
    if (store_stage_held())                          /* boot upload is using the store buffer */
        return -8;
#endif
    capture_settings(&proj.set);
    if (measure_project(&proj, &packed_len))
        return -7;
    if (save_obj(OBJ_SET, &proj.set, sizeof proj.set))
        return -1;
    if (save_obj(OBJ_SOUND, &proj.sound, sizeof proj.sound))
        return -2;
    return save_stream();
}

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
