/* SPDX-License-Identifier: GPL-3.0-only */
/* Sound-only reset: a saved part returns to its factory image, mixer
 * level, pan, sends and mute stay put, and undo restores the saved sound.
 * An empty part returns SOUND_RESET_NO_SAVED and writes nothing. */
#include <stdio.h>
#include <string.h>
#include "../../firmware/src/app/undo.c"
#include "../../firmware/src/app/sound_reset.c"

static int fails;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static int span_of(int part, int *target, int *voice, int *nvoice, int *tag)
{
    *target = 0;
    *voice = 0;
    *nvoice = 1;
    *tag = -1;
    switch (part) {
    case PART_909: *target = T_909; *nvoice = NVOICES_MAX; return 0;
    case PART_808: *target = T_808; *nvoice = NVOICES_MAX; return 0;
    case PART_303A: *target = T_303; *tag = 0; return 0;
    case PART_303B: *target = T_303; *voice = 1; *tag = 1; return 0;
    case PART_BRK: *target = T_BRK; return 0;
    default: return -1;
    }
}

static void paint_part(sound_t *s, int part, uint8_t seed)
{
    int target, voice, nvoice, tag, v, i;
    span_of(part, &target, &voice, &nvoice, &tag);
    for (v = 0; v < nvoice; v++)
        for (i = 0; i < NPARAMS_MAX; i++)
            s->v[target][voice + v][i] = (uint8_t)(seed + v + i);
    if (tag >= 0)
        for (i = 0; i < (int)sizeof s->bass[tag].bytes; i++)
            s->bass[tag].bytes[i] = (uint8_t)(seed + 200 + i);
}

static void clear_part(sound_t *s, int part)
{
    int target, voice, nvoice, tag, v;
    span_of(part, &target, &voice, &nvoice, &tag);
    for (v = 0; v < nvoice; v++)
        memset(s->v[target][voice + v], 0, NPARAMS_MAX);
    if (tag >= 0)
        memset(s->bass[tag].bytes, 0, sizeof s->bass[tag].bytes);
}

static int part_is(const sound_t *a, const sound_t *b, int part)
{
    int target, voice, nvoice, tag, v;
    span_of(part, &target, &voice, &nvoice, &tag);
    for (v = 0; v < nvoice; v++)
        if (memcmp(a->v[target][voice + v], b->v[target][voice + v], NPARAMS_MAX) != 0)
            return 0;
    if (tag >= 0 && memcmp(a->bass[tag].bytes, b->bass[tag].bytes, sizeof a->bass[tag].bytes) != 0)
        return 0;
    return 1;
}

static void paint_mixer(sound_t *s, int part, uint8_t level, uint8_t rev, uint8_t dly, uint8_t pan, uint8_t cut)
{
    s->v[T_MIX][part][MX_LEVEL] = level;
    s->v[T_MIX][part][MX_REV] = rev;
    s->v[T_MIX][part][MX_DLY] = dly;
    s->v[T_MIX][part][MX_PAN] = pan;
    s->v[T_MIX][part][MX_CUT] = cut;
}

static void saved_part(int part)
{
    sound_t live, factory, snap;
    uint8_t shadow[sizeof live], mute, peek[NPARAMS_MAX];
    undo_t undo;
    uint32_t lo, hi, cell;
    int target, voice, nvoice, tag, rc;
    memset(&live, 0x5a, sizeof live);
    memset(&factory, 0x11, sizeof factory);
    paint_part(&live, part, 0x30);
    paint_part(&factory, part, 0x80);
    paint_mixer(&live, part, 0x21, 0x32, 0x43, 0x54, 0x65);
    paint_mixer(&factory, part, 0x01, 0x02, 0x03, 0x04, 0x05);
    mute = (uint8_t)(0x76 + part);
    snap = live;
    undo_init(&undo, &live, shadow, sizeof live);
    rc = sound_reset_part(&live, &factory, part, &mute, &undo);
    CHECK(rc == SOUND_RESET_OK, "part %d reset status %d", part, rc);
    CHECK(part_is(&live, &factory, part), "part %d sound cells are not the factory image", part);
    CHECK(live.v[T_MIX][part][MX_LEVEL] == 0x21, "part %d mixer level changed", part);
    CHECK(live.v[T_MIX][part][MX_REV] == 0x32, "part %d mixer rev send changed", part);
    CHECK(live.v[T_MIX][part][MX_DLY] == 0x43, "part %d mixer dly send changed", part);
    CHECK(live.v[T_MIX][part][MX_PAN] == 0x54, "mixer pan changed part %d", part);
    CHECK(live.v[T_MIX][part][MX_CUT] == 0x65, "part %d mixer cutoff changed", part);
    CHECK(mute == (uint8_t)(0x76 + part), "part %d mixer mute changed", part);
    CHECK(memcmp(live.v[T_MIX], snap.v[T_MIX], sizeof live.v[T_MIX]) == 0, "part %d mixer plane changed", part);
    CHECK(memcmp(live.v[T_FX], snap.v[T_FX], sizeof live.v[T_FX]) == 0, "part %d fx changed", part);
    CHECK(memcmp(live.v[T_MST], snap.v[T_MST], sizeof live.v[T_MST]) == 0, "part %d master changed", part);
    if (part == PART_BRK)
        CHECK(memcmp(live.v[T_BRK][1], snap.v[T_BRK][1], NPARAMS_MAX) == 0,
              "break reset wrote a second voice");
    {
        int other;
        for (other = 0; other < NPARTS; other++)
            if (other != part)
                CHECK(part_is(&live, &snap, other), "part %d reset wrote part %d", part, other);
    }
    CHECK(undo.n == 1 && undo.cur == 1, "part %d undo entry missing", part);
    span_of(part, &target, &voice, &nvoice, &tag);
    cell = (uint32_t)((uint8_t *)&live.v[target][voice][0] - (uint8_t *)&live);
    undo_peek(&undo, 0, cell, peek, NPARAMS_MAX);
    CHECK(memcmp(peek, snap.v[target][voice], NPARAMS_MAX) == 0, "part %d undo peek is not the saved sound", part);
    CHECK(undo_undo(&undo, 0, &lo, &hi) == 1, "part %d undo failed", part);
    CHECK(memcmp(&live, &snap, sizeof live) == 0, "part %d undo did not restore the saved sound", part);
    CHECK(mute == (uint8_t)(0x76 + part), "part %d mute changed during undo", part);
    CHECK(undo_redo(&undo, 0, &lo, &hi) == 1, "part %d redo failed", part);
    CHECK(part_is(&live, &factory, part), "part %d redo is not the factory image", part);
    CHECK(live.v[T_MIX][part][MX_PAN] == 0x54, "mixer pan changed part %d after redo", part);
}

static void no_saved(int part)
{
    sound_t live, factory, snap;
    uint8_t shadow[sizeof live], mute;
    undo_t undo;
    uint32_t mix;
    int rc;
    memset(&live, 0x3c, sizeof live);
    memset(&factory, 0x22, sizeof factory);
    clear_part(&live, part);
    paint_mixer(&live, part, 0x11, 0x22, 0x33, 0x44, 0x55);
    mute = 0x99;
    snap = live;
    undo_init(&undo, &live, shadow, sizeof live);
    mix = (uint32_t)((uint8_t *)&live.v[T_MIX][part][MX_LEVEL] - (uint8_t *)&live);
    shadow[mix] ^= 0xff; /* a commit would record this mixer byte */
    rc = sound_reset_part(&live, &factory, part, &mute, &undo);
    CHECK(rc == SOUND_RESET_NO_SAVED, "part %d empty status %d", part, rc);
    CHECK(rc != SOUND_RESET_OK, "part %d empty status is not distinct", part);
    CHECK(memcmp(&live, &snap, sizeof live) == 0, "part %d empty path mutated the sound", part);
    CHECK(mute == 0x99, "part %d empty path mutated mixer mute", part);
    CHECK(undo.n == 0 && undo.cur == 0 && undo.used == 0, "part %d empty path wrote undo", part);
}

/* One nonzero byte in an otherwise zero part. A 303 tag byte, or parameter
 * index NPARAMS_MAX-1, is a saved sound and the reset must run. */
static void one_saved_byte(int part, int tag_byte, int last_voice)
{
    sound_t live, factory, snap;
    uint8_t shadow[sizeof live], mute = 0x77;
    undo_t undo;
    uint32_t lo, hi;
    int target, voice, nvoice, tag, rc;
    const char *name;

    memset(&live, 0, sizeof live);
    memset(&factory, 0, sizeof factory);
    span_of(part, &target, &voice, &nvoice, &tag);
    if (last_voice)
        voice += nvoice - 1;
    if (tag_byte) {
        name = "bass-tag";
        live.bass[tag].bytes[BASS_SOUND_TAG_BYTES - 1] = 0x3c;
        factory.bass[tag].bytes[BASS_SOUND_TAG_BYTES - 1] = 0x91;
    } else {
        name = "last-parameter";
        live.v[target][voice][NPARAMS_MAX - 1] = 0x3c;
        factory.v[target][voice][NPARAMS_MAX - 1] = 0x91;
    }
    paint_mixer(&live, part, 0x21, 0x32, 0x43, 0x54, 0x65);
    snap = live;
    undo_init(&undo, &live, shadow, sizeof live);
    rc = sound_reset_part(&live, &factory, part, &mute, &undo);
    CHECK(rc == SOUND_RESET_OK, "%s reset status %d", name, rc);
    CHECK(rc != SOUND_RESET_NO_SAVED, "%s stayed NO_SAVED", name);
    CHECK(part_is(&live, &factory, part), "%s was not reset", name);
    CHECK(memcmp(live.v[T_MIX], snap.v[T_MIX], sizeof live.v[T_MIX]) == 0, "%s moved the mixer", name);
    CHECK(memcmp(live.v[T_FX], snap.v[T_FX], sizeof live.v[T_FX]) == 0, "%s moved fx", name);
    CHECK(memcmp(live.v[T_MST], snap.v[T_MST], sizeof live.v[T_MST]) == 0, "%s moved master", name);
    CHECK(mute == 0x77, "%s changed mute", name);
    CHECK(undo.n == 1 && undo.cur == 1, "%s stored no undo", name);
    CHECK(undo_undo(&undo, 0, &lo, &hi) == 1, "%s undo failed", name);
    CHECK(memcmp(&live, &snap, sizeof live) == 0, "%s undo did not restore the single byte", name);
}

/* A dirty tail past the sound makes the undo diff larger than the pool.
 * BAD must leave this part as it was. undo.c still clears the pool. */
static void oversize_undo(int part)
{
    enum { GAP = (int)UNDO_GAP + 1, TAIL = (int)UNDO_POOL };
    struct holder {
        sound_t sound;
        uint8_t gap[GAP];
        uint8_t tail[TAIL];
    };
    static struct holder hold, image;
    static uint8_t shadow_buf[sizeof(struct holder)];
    sound_t factory;
    undo_t undo;
    uint8_t mute = 0x76;
    int rc;

    memset(&hold, 0x5a, sizeof hold);
    memset(&factory, 0x11, sizeof factory);
    paint_part(&hold.sound, part, 0x30);
    paint_mixer(&hold.sound, part, 0x21, 0x32, 0x43, 0x54, 0x65);
    undo_init(&undo, &hold, shadow_buf, sizeof hold);
    memset(hold.tail, 0xa5, sizeof hold.tail);
    image = hold;
    rc = sound_reset_part(&hold.sound, &factory, part, &mute, &undo);
    CHECK(rc == SOUND_RESET_BAD, "part %d oversize diff status %d", part, rc);
    CHECK(part_is(&hold.sound, &image.sound, part), "part %d oversize diff kept the factory cells", part);
    CHECK(memcmp(hold.sound.v[T_MIX], image.sound.v[T_MIX], sizeof hold.sound.v[T_MIX]) == 0,
          "part %d oversize diff changed the mixer", part);
    CHECK(memcmp(hold.sound.v[T_FX], image.sound.v[T_FX], sizeof hold.sound.v[T_FX]) == 0,
          "part %d oversize diff changed fx", part);
    CHECK(memcmp(hold.sound.v[T_MST], image.sound.v[T_MST], sizeof hold.sound.v[T_MST]) == 0,
          "part %d oversize diff changed master", part);
    CHECK(mute == 0x76, "part %d oversize diff changed mute", part);
    CHECK(undo.n == 0 && undo.cur == 0 && undo.used == 0, "part %d oversize diff kept undo history", part);
    CHECK(hold.tail[0] == 0xa5, "part %d oversize diff rewrote bytes outside the sound", part);
    CHECK(part_is(&hold.sound, (const sound_t *)shadow_buf, part),
          "part %d oversize diff left the part pending", part);
}

/* Finish the prior gesture before reset, then undo each gesture separately. */
static void committed_sound_edit(int part)
{
    sound_t live, factory, edited;
    uint8_t shadow[sizeof live], mute = 0x39;
    undo_t undo;
    uint32_t lo, hi;
    memset(&factory, 0x22, sizeof factory);
    live = factory;
    undo_init(&undo, &live, shadow, sizeof live);
    paint_part(&live, part, 0x61);
    edited = live;
    CHECK(undo_commit(&undo) == 1, "part %d prior gesture not committed", part);
    CHECK(sound_reset_part(&live, &factory, part, &mute, &undo) == SOUND_RESET_OK,
          "part %d committed edit reset failed", part);
    CHECK(undo.n == 2 && undo.cur == 2, "part %d reset is not a separate gesture", part);
    CHECK(undo_undo(&undo, 0, &lo, &hi) == 1, "part %d reset undo failed", part);
    CHECK(memcmp(&live, &edited, sizeof live) == 0, "part %d reset lost committed edit", part);
    CHECK(undo_undo(&undo, 0, &lo, &hi) == 1, "part %d prior gesture undo failed", part);
    CHECK(memcmp(&live, &factory, sizeof live) == 0, "part %d prior baseline lost", part);
    CHECK(undo_redo(&undo, 0, &lo, &hi) == 1, "part %d prior gesture redo failed", part);
    CHECK(memcmp(&live, &edited, sizeof live) == 0, "part %d prior edit redo changed", part);
    CHECK(undo_redo(&undo, 0, &lo, &hi) == 1, "part %d reset redo failed", part);
    CHECK(memcmp(&live, &factory, sizeof live) == 0, "part %d reset redo changed", part);
    CHECK(mute == 0x39, "part %d gesture changed mute", part);
}

/* A no-op must preserve an existing redo entry, shadow and active pool. */
static void unchanged_history(int part, int empty)
{
    sound_t live, factory, snap;
    uint8_t shadow[sizeof live], shadow_snap[sizeof live], mute = 0x72;
    undo_t undo, undo_snap;
    uint32_t lo, hi;
    memset(&live, 0x22, sizeof live);
    if (empty)
        clear_part(&live, part);
    factory = live;
    undo_init(&undo, &live, shadow, sizeof live);
    live.v[T_MIX][part][MX_PAN] ^= 1;
    CHECK(undo_commit(&undo) == 1, "part %d history fixture commit failed", part);
    CHECK(undo_undo(&undo, 0, &lo, &hi) == 1, "part %d history fixture undo failed", part);
    snap = live;
    memcpy(shadow_snap, shadow, sizeof shadow);
    memcpy(&undo_snap, &undo, sizeof undo);
    CHECK(sound_reset_part(&live, &factory, part, &mute, &undo) ==
          (empty ? SOUND_RESET_NO_SAVED : SOUND_RESET_OK), "part %d no-op status", part);
    CHECK(memcmp(&live, &snap, sizeof live) == 0, "part %d no-op changed sound", part);
    CHECK(memcmp(shadow, shadow_snap, sizeof shadow) == 0, "part %d no-op changed shadow", part);
    CHECK(memcmp(&undo, &undo_snap, sizeof undo) == 0, "part %d no-op changed history", part);
    CHECK(mute == 0x72, "part %d no-op changed mute", part);
    CHECK(undo_redo(&undo, 0, &lo, &hi) == 1, "part %d no-op discarded redo", part);
    CHECK(live.v[T_MIX][part][MX_PAN] == (uint8_t)(snap.v[T_MIX][part][MX_PAN] ^ 1),
          "part %d preserved redo is incorrect", part);
}

static void bad_args(void)
{
    sound_t live, factory, snap;
    uint8_t shadow[sizeof live], mute = 0xab, other_region[sizeof live];
    undo_t undo;
    memset(&live, 0x44, sizeof live);
    memset(&factory, 0x55, sizeof factory);
    paint_part(&live, PART_909, 0x30);
    snap = live;
    undo_init(&undo, &live, shadow, sizeof live);
    CHECK(sound_reset_part(0, &factory, PART_909, &mute, &undo) == SOUND_RESET_BAD, "null sound");
    CHECK(sound_reset_part(&live, 0, PART_909, &mute, &undo) == SOUND_RESET_BAD, "null factory");
    CHECK(sound_reset_part(&live, &factory, PART_909, 0, &undo) == SOUND_RESET_BAD, "null mute");
    CHECK(sound_reset_part(&live, &factory, PART_909, &mute, 0) == SOUND_RESET_BAD, "null undo");
    CHECK(sound_reset_part(&live, &live, PART_909, &mute, &undo) == SOUND_RESET_BAD, "aliased factory");
    CHECK(sound_reset_part(&live, &factory, -1, &mute, &undo) == SOUND_RESET_BAD, "part -1");
    CHECK(sound_reset_part(&live, &factory, NPARTS, &mute, &undo) == SOUND_RESET_BAD, "part NPARTS");
    undo_init(&undo, other_region, shadow, sizeof live);
    CHECK(sound_reset_part(&live, &factory, PART_909, &mute, &undo) == SOUND_RESET_BAD, "region is not the sound");
    undo_init(&undo, &live, shadow, sizeof live - 1);
    CHECK(sound_reset_part(&live, &factory, PART_909, &mute, &undo) == SOUND_RESET_BAD, "short region");
    CHECK(memcmp(&live, &snap, sizeof live) == 0, "bad args mutated the sound");
    CHECK(mute == 0xab, "bad args mutated mute");
}

int main(void)
{
    int part;
    CHECK(SOUND_RESET_NO_SAVED != SOUND_RESET_OK, "status codes collide");
    for (part = 0; part < NPARTS; part++) {
        saved_part(part);
        no_saved(part);
        committed_sound_edit(part);
        unchanged_history(part, 0);
        unchanged_history(part, 1);
    }
    one_saved_byte(PART_909, 0, 0);
    one_saved_byte(PART_909, 0, 1);
    one_saved_byte(PART_808, 0, 1);
    one_saved_byte(PART_303A, 1, 0);
    oversize_undo(PART_909);
    oversize_undo(PART_303A);
    bad_args();
    if (fails)
        printf("sound-reset: %d FAILED\n", fails);
    else
        printf("sound-reset: ok\n");
    return fails != 0;
}
