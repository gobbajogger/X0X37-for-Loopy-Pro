
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
#define PROJ_MAGIC 0x50305830u           /* "0X0P" */
#define PROJ_FORMAT 1u
#define PROJ_STEREO 2u
#if X0X_ORIGINAL_SYNTH
#define PROJ_SND_REV 13u
#elif X0X_BASS_TAGGED_SOUND
#define PROJ_SND_REV 11u
#else
#define PROJ_SND_REV 10u
#endif
#define PROJ_BT_CLK_IN 8u
#define PROJ_BT_SMC_CENTRAL 16u  /* connect to an SMC-Mixer instead of advertising at next boot */
#define PROJ_BT_SMC_USER 32u     /* SMC's editable User preset, rather than factory DAW mode */
#define PROJ_DEVICE_BT (1u | PROJ_BT_SMC_CENTRAL | PROJ_BT_SMC_USER)
typedef struct {
    uint32_t magic, format;
    uint8_t palette, keyled, clk_out, notes_out;   /* keyled: 0 off, 1 keys and the buttons' glow, 2 keys only */
    uint16_t bpm_x10;
    uint8_t accent_q7, keysound;         /* black drum keys sound: 0 = only when stopped or recording, 1 = always */
    uint8_t autosave_off;                /* 0: save by itself while stopped and idle (ui.c autosave) */
    uint8_t rsv_bright;                  /* was the screen's brightness (removed: always full); unused */
    uint8_t stereo;                      /* the pans the sound has: 0 none (saved mono), 1 the parts', 2 the
                                          * drum voices' too (PROJ_STEREO); a load centres what is missing */
    uint8_t snd_rev;                     /* the sound's layout (PROJ_SND_REV): 0 = before the 909 SD's DECAY */
    uint8_t midi_ch[5];                  /* each part's MIDI channel (in and out): 0 = its default (an older
                                          * project), 1-16, 17 = OFF */
    uint8_t out_loud;                    /* 1: the output 6 dB hotter (the codec's full scale) */
    uint8_t usb_full;                    /* 1: USB audio at full level, the MASTER knob for the FM-1's own outputs */
    uint8_t bt;                          /* bit 0: Bluetooth MIDI on (SMK-37; 0 = off, also in older objects);
                                          * bits 1-3: CLOCK SOURCE (PROJ_BT_CLK_IN) */
    uint8_t ck_ch;                       /* SMK-37 clip mode: the keybed MIDI channel 2-15 (0: an older object) */
    uint8_t ck_root, ck_scale, ck_snap;  /* clip mode SCALE: root 0-11, scale, 0 NEAR / 1 FILTER (smk_clip_keys.h) */
    uint8_t ck_chord;                    /* clip mode CHORD: type */
    uint8_t ck_ord, ck_rate, ck_oct;     /* clip mode ARP: order, rate, octaves - 1 (0-3) */
    uint8_t ck_gate;                     /* clip mode ARP: gate / 10 % - 1 (0-9) */
    uint8_t ck_inv;                      /* clip mode CHORD: inversion 0-3 */
    uint8_t ck_x1;                       /* clip mode ARP: bits 0-2 SWING (50 + 3 x %), bits 3-5 HUMAN (0 = off) */
    uint8_t ck_x2;                       /* clip mode: bits 0-3 ARP RHYTHM (CK_RHY_T), bits 4-6 CHORD STRUM (x 10 ms) */
} settings_t;
#define PROJ_STREAM 13u                 /* compact Chicago schema2; current raw patterns/arrangement */
static int stream_known(uint32_t format) { return format == PROJ_STREAM; }
#define PROJ_STREAM_PREV 3u             /* velocity steps, legacy gates */
#define PROJ_STREAM_OLD 2u
static const uint8_t STREAM_OBJ[] = {OBJ_PAT0, OBJ_PAT1, OBJ_SONG, OBJ_MOTION};
#define NSTREAM ((int)sizeof STREAM_OBJ)
#define STREAM_HDR 16u                  /* in the first object: format, raw length, raw CRC, packed length */
typedef struct { settings_t set; int sound; } project_t;
static project_t proj;
static uint8_t settings_read_failed, settings_load_failed, project_incompatible;
static uint8_t stream_previous, stored_known[OBJ_NOBJ], io_buf[256];
static uint64_t stored_hash[OBJ_NOBJ];
static uint32_t packed_len;
static int settings_result, sound_result, stream_result, validate_result, checks;
static unsigned sound_reads, stream_reads, validations;
static settings_t saved;
__attribute__((noinline)) static uint64_t hash_of(const uint8_t *p, uint32_t n)        /* FNV-1a, 64-bit */
{
    uint64_t h = UINT64_C(14695981039346656037);
    while (n--)
        h = (h ^ *p++) * UINT64_C(1099511628211);
    return h;
}
__attribute__((noinline)) static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
__attribute__((noinline)) static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static int plat_store_load(unsigned obj, void *out, unsigned capacity) {
    assert(obj == OBJ_SET && out == io_buf && capacity >= sizeof saved + 4u);
    if (settings_result) return settings_result;
    put32(io_buf, PROJ_FORMAT);
    memcpy(io_buf + 4u, &saved, sizeof saved);
    return (int)sizeof saved + 4;
}
static int read_settings_payload(void)
{
    int n = plat_store_load(OBJ_SET, io_buf, sizeof io_buf);
    settings_read_failed = (uint8_t)(n < -1);
    if (n < 0) stored_known[OBJ_SET] = 0;
    return n;
}
static int load_settings(settings_t *dst)
{
    int n = read_settings_payload();
    if (n < 0) return n;
    if ((uint32_t)n != sizeof *dst + 4u || get32(io_buf) != PROJ_FORMAT ||
        io_buf[4u + __builtin_offsetof(settings_t, snd_rev)] != PROJ_SND_REV) return -9;
    __builtin_memcpy(dst, io_buf + 4u, sizeof *dst);
    stored_known[OBJ_SET] = 1;
    stored_hash[OBJ_SET] = hash_of(io_buf, (uint32_t)n);
    return 0;
}
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
    s->bt = (uint8_t)((device_bt & PROJ_DEVICE_BT) | PROJ_BT_CLK_IN);
}
void project_defaults(void) { project_defaults_for(&proj, proj.set.bt); }
static int read_project(project_t *pr)
{
    uint8_t device_bt = proj.set.bt;
    project_defaults_for(pr, device_bt);
    packed_len = 0;
    settings_t set = pr->set;
    /* #790 + #810: -2/-8 are retryable read failures (block saves with -8);
     * any other non-erased failure is damaged/committed data (#810, -9). */
    int bad = 0, present = 0, transient = 0, rc = load_settings(&set);
    present |= rc != -1;
    transient |= rc == -2 || rc == -8;
    int settings_present = rc == 0;
    if (settings_read_failed) {
        settings_load_failed = 1;
        goto failed;
    }
    if (rc == -9) goto unsupported;
    if (!rc) {
        if (set.magic != PROJ_MAGIC || set.format != PROJ_FORMAT ||
            set.snd_rev != PROJ_SND_REV || set.stereo != PROJ_STEREO) goto unsupported;
        pr->set = set;
        device_bt = set.bt;                       /* retain saved Bluetooth role on partial-load fallback */
    } else {
        bad = 1;
#if BOARD_SMK37
        /* No stored device settings: connect an SMC on the very first boot.
         * Valid saved OFF/DEVICE settings are never overwritten. */
        if (rc == -1 && !(device_bt & PROJ_DEVICE_BT))
            device_bt = (uint8_t)(1u | PROJ_BT_SMC_CENTRAL);
#endif
    }
    rc = load_sound(&pr->sound);
    present |= rc != -1;
    transient |= rc == -2 || rc == -8;
    if (rc == -9) goto unsupported;
    if (rc) bad = 1;
    stream_previous = 0;
    int n = load_stream_object(0);
    present |= n != -1;
    transient |= n == -2 || n == -8;
    if (n >= 0 && (n < 4 || !stream_known(get32(io_buf)))) goto unsupported;
    if (n >= 4) {
        rc = load_stream(pr, n, get32(io_buf));
        if (rc == -9) goto unsupported;
        /* At most 16 A/B combinations, no second stream buffer. A cut can
         * leave only some newest objects changed (unchanged objects are not
         * written), so taking all older copies is not sufficient. */
        for (unsigned mask = 1; rc && mask < (1u << NSTREAM); ++mask) {
            stream_previous = (uint8_t)mask;
            n = load_stream_object(0);
            if (n < (int)STREAM_HDR || !stream_known(get32(io_buf))) continue;
            rc = load_stream(pr, n, get32(io_buf));
            if (!rc) rc = project_validate_for(pr);
        }
        if (!rc && stream_previous)
            for (int k = 0; k < NSTREAM; ++k) stored_known[STREAM_OBJ[k]] = 0;
        stream_previous = 0;
        if (rc) {
            patterns_defaults(pr);
            arrange_defaults(pr);
            bad = 1;
        }
    } else {
        bad = 1;
        /* PAT0 may itself be missing while later committed segments remain. */
        for (int k = 1; k < NSTREAM; ++k)
        {
            int m = load_stream_object(k);
            present |= m != -1;
            transient |= m == -2 || m == -8;
        }
    }
    rc = bad ? -1 : project_validate_for(pr);
    if (!rc) return 0;
    goto failed;
unsupported:
    rc = -9;
failed:
    /* A later sound/stream failure also discards the loaded settings from pr.
     * Do not let those fallback defaults replace the original settings. */
    if (settings_present) settings_load_failed = 1;
    project_defaults_for(pr, device_bt);
    packed_len = 0;
    stream_previous = 0;
    if (rc != -9 && present && !transient) project_incompatible = 2;
    if (transient) settings_load_failed = 1;
    return rc;
}
int project_load(void)
{
    project_incompatible = 0;
    int rc = read_project(&proj);
    if (!rc) settings_load_failed = 0;
    if (rc == -9
#if X0X_BASS_TAGGED_SOUND
        || (!rc && engine_bass_sound_validate(&proj.sound))
#endif
        ) project_incompatible = 1;
    return rc;
}
/* Only the real settings fence is exercised; this is not a full save test. */
static int settings_save_gate(void) {
    if (settings_read_failed || settings_load_failed) return -8; /* retry load before saving fallback defaults */
    return 0;
}
static void simulate_boot(void) {
    if (project_load() != 0)
        project_defaults();                         /* nothing saved yet (or another format) */
}
static unsigned n_events, prepare_arg, on_arg;
static char events[8];
static void ble_prepare(unsigned on) { events[n_events++] = 'P';prepare_arg=on; }
static void ble_start(void) { events[n_events++] = 'S'; }
static void plat_ble_set(unsigned on) { events[n_events++] = 'T';on_arg=on; }
static void bluetooth_boot(void) {
    /* Initialize the checked controller at boot even if the saved radio setting is OFF.
     * Keep the image/boot-loop guards: an OFF-to-ON toggle later needs no restart. */
    ble_prepare(1);
    ble_start();
    plat_ble_set(proj.set.bt & 1u);                 /* OFF stays idle until explicitly enabled */
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
