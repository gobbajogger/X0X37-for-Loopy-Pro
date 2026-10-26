/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual product metadata + actual rev10 export admission; no schema/source adoption. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
/* Metadata-only access to canonical WBL1 identifiers in a build where unavailable.
 * engine.c is separately compiled with the real build flag, not this test override. */
#define X0X_SYNTH_HOST_TEST 1
#include "../../firmware/src/dsp/wobble_backend.h"
#include "../../firmware/src/app/bass_identity.h"
#include <assert.h>

static unsigned checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#x); exit(1); } } while(0)
static const bass_identity_entry_t registry[] = {
    {SYNTH_ENGINE_303, BASS_IDENTITY_PRODUCT_SCHEMA, CHI_AUX_END, 1},
    {WOBBLE_EXPERIMENT_ENGINE, BASS_IDENTITY_PRODUCT_SCHEMA, WOBBLE_EXPERIMENT_NPARAMS, X0X_DEV_WOBBLE != 0}
};
#define NREG (sizeof registry / sizeof *registry)
static void codec(void)
{
    bass_identity_t native = {SYNTH_ENGINE_303, BASS_IDENTITY_PRODUCT_SCHEMA, CHI_AUX_END, BASS_IDENTITY_KNOWN};
    bass_identity_t wobble = {WOBBLE_EXPERIMENT_ENGINE, BASS_IDENTITY_PRODUCT_SCHEMA, WOBBLE_EXPERIMENT_NPARAMS, BASS_IDENTITY_KNOWN};
    bass_identity_t out, bad, sentinel = {99, 99, 99, 99};
    uint8_t wire[8], before[8];
    int ws = X0X_DEV_WOBBLE ? BASS_IDENTITY_READY : BASS_IDENTITY_UNAVAILABLE;
    CHECK(SYNTH_ENGINE_303 == synth303_ops.id); CHECK(SYNTH_SCHEMA_303 == synth303_ops.schema);
    CHECK(WOBBLE_EXPERIMENT_ENGINE == wobble_foundation_ops.id);
    CHECK(WOBBLE_EXPERIMENT_SCHEMA == wobble_foundation_ops.schema);
    CHECK(synth303_ops.nparams == BASS303_NPARAMS && CHI_AUX_END > BASS303_NPARAMS);
    CHECK(wobble_foundation_ops.nparams == WOBBLE_EXPERIMENT_NPARAMS);
    CHECK(bass_identity_encode(wire, sizeof wire, &native, registry, NREG) == BASS_IDENTITY_READY);
    CHECK(wire[0] == 1 && wire[1] == 0 && wire[4] == 1 && wire[6] == 32 && wire[7] == 1);
    CHECK(bass_identity_decode(&out, wire, sizeof wire, registry, NREG) == BASS_IDENTITY_READY);
    CHECK(out.engine == native.engine && out.count == native.count);
    CHECK(bass_identity_encode(wire, sizeof wire, &wobble, registry, NREG) == ws);
    CHECK(wire[0] == 0x31 && wire[1] == 0x4c && wire[2] == 0x42 && wire[3] == 0x57 && wire[6] == 23);
    CHECK(bass_identity_decode(&out, wire, sizeof wire, registry, NREG) == ws);
    for (unsigned i = 0; i <= 10; i++) {
        CHECK(bass_identity_legacy(i, &out) == BASS_IDENTITY_CHOICE_REQUIRED);
        CHECK(!out.engine && !out.schema && !out.count && out.state == BASS_IDENTITY_UNKNOWN);
        CHECK(bass_identity_encode(wire, sizeof wire, &out, registry, NREG) == BASS_IDENTITY_CHOICE_REQUIRED);
        for (unsigned j = 0; j < sizeof wire; j++) CHECK(!wire[j]);
    }
    out = sentinel; CHECK(bass_identity_legacy(11, &out) == BASS_IDENTITY_UNSUPPORTED);
    CHECK(!memcmp(&out, &sentinel, sizeof out));
    for (unsigned field = 0; field < 6; field++) {
        bad = native;
        if (field == 0) bad.engine = 0;
        if (field == 1) bad.schema = 0;
        if (field == 2) bad.count = 0;
        if (field == 3) bad.count--;
        if (field == 4) bad.state = 2;
        if (field == 5) bad.state = BASS_IDENTITY_UNKNOWN;
        memset(wire, 0x5a, sizeof wire); memcpy(before, wire, sizeof wire);
        CHECK(bass_identity_check(&bad, registry, NREG) == BASS_IDENTITY_INVALID);
        CHECK(bass_identity_encode(wire, sizeof wire, &bad, registry, NREG) == BASS_IDENTITY_INVALID);
        CHECK(!memcmp(wire, before, sizeof wire));
    }
    bad = native; bad.schema++;
    CHECK(bass_identity_check(&bad, registry, NREG) == BASS_IDENTITY_UNSUPPORTED);
    bad = native; bad.engine = 0x12345678;
    CHECK(bass_identity_check(&bad, registry, NREG) == BASS_IDENTITY_UNSUPPORTED);
    memcpy(before, wire, sizeof wire);
    CHECK(bass_identity_encode(wire, sizeof wire, &bad, registry, NREG) == BASS_IDENTITY_UNSUPPORTED);
    CHECK(!memcmp(wire, before, sizeof wire));
    out = sentinel;
    CHECK(bass_identity_decode(&out, wire, 7, registry, NREG) == BASS_IDENTITY_INVALID);
    CHECK(!memcmp(&out, &sentinel, sizeof out));
    bass_identity_entry_t conflicting[2] = {registry[0], registry[0]}; conflicting[1].count--;
    CHECK(bass_identity_check(&native, conflicting, 2) == BASS_IDENTITY_INVALID);
    CHECK(bass_identity_check(&native, NULL, 1) == BASS_IDENTITY_INVALID);
    CHECK(bass_identity_check(NULL, registry, NREG) == BASS_IDENTITY_INVALID);
}
static void actual_dispatch(void)
{
    boot();
    for (int v = 0; v < NBASS; v++) {
        int wobble = engine_bass_is_wobble(v);
        int count = engine_nparams(T_303, v);
        const bass_identity_entry_t *r = &registry[wobble ? 1 : 0];
        CHECK(count == r->count); CHECK(r->available == 1);
        for (int i = 0; i < count; i++) {
            const x0x_param_t *p = engine_param(T_303, v, i);
            CHECK(p && p->name && p->def <= p->max);
        }
        CHECK(engine_param(T_303, v, count) == NULL);
    }
}
static void legacy_export(const char *path)
{
    FILE *f = fopen(path,"rb"); long length;
    uint8_t *b, *copy; bass_identity_t tags[NBASS];
    CHECK(f != NULL); CHECK(!fseek(f,0,SEEK_END)); length=ftell(f); CHECK(length>32); rewind(f);
    b=malloc((size_t)length); copy=malloc((size_t)length); CHECK(b && copy);
    CHECK(fread(b,1,(size_t)length,f)==(size_t)length); fclose(f); memcpy(copy,b,(size_t)length);
    CHECK(get32(b)==0x50583058u && get32(b+4)==1 && get32(b+16)==PROJ_FORMAT);
    CHECK(get32(b+20)==3 || get32(b+20)==6); CHECK(get32(b+24)==10);
    CHECK(get32(b+8)==(uint32_t)length-32 && get32(b+28)==0);
    CHECK(get32(b+12)==crc32_of(b+32,(uint32_t)length-32));
    for (int v=0; v<NBASS; v++) {
        CHECK(bass_identity_legacy(get32(b+24),&tags[v])==BASS_IDENTITY_CHOICE_REQUIRED);
        CHECK(bass_identity_check(&tags[v],registry,NREG)==BASS_IDENTITY_CHOICE_REQUIRED);
    }
    CHECK(!memcmp(copy,b,(size_t)length));
    printf("PRESERVED rev10 export stream%u len%ld CRC%08x: both voices CHOICE_REQUIRED\n",get32(b+20),length,get32(b+12));
    free(copy); free(b);
}
int main(int argc,char **argv)
{
    if (argc < 3) { fputs("Supply actual stream3/10 and6/10 export fixtures.\n", stderr); return 2; }
    codec(); actual_dispatch();
    for (int i=1;i<argc;i++) legacy_export(argv[i]);
    printf("PASS %u identity/admission/production-dispatch checks, WBL1 available=%d; unintegrated only\n",checks,X0X_DEV_WOBBLE != 0);
    return 0;
}
