/* Diagnostic regression: use the production boot, engine, UI and renderer. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
#include <assert.h>
#include "../../experiments/wobble/wobble.h"

int main(int argc, char **argv)
{
    boot();
    printf("WBL1 active: %d; carrier-wave value=%u, maximum=%u\n",
        engine_bass_is_wobble(0), proj.sound.v[T_303][0][W_CARRIER_WAVE],
        engine_param(T_303, 0, W_CARRIER_WAVE)->max);
    fflush(stdout);
    assert(engine_bass_is_wobble(0) == !!X0X_DEV_WOBBLE);
    if (argc > 1 && !strcmp(argv[1], "check-defaults")) {
        for (int v=0;v<NBASS;v++)
            for (int i=0;i<engine_nparams(T_303,v);i++) {
                const x0x_param_t *p=engine_param(T_303,v,i);
                if (p && proj.sound.v[T_303][v][i]>p->max) {
                    fprintf(stderr,"Invalid voice %d parameter %d (%s): %u > %u\n",
                        v,i,p->name,proj.sound.v[T_303][v][i],p->max);
                    return 1;
                }
            }
        return 0;
    }
    for (int v=0;v<NBASS;v++) {
        if (!engine_bass_is_wobble(v)) {
            assert(proj.sound.v[T_303][v][BASS303_CUTOFF]==48);
            assert(proj.sound.v[T_303][v][BASS303_DRVTYPE]==2);
        }
        ui.part=PART_303A+v;
        set_view(V_PART);
        ui.touched=-1;
        ui.msg_until=0;
        ui_frame();
    }
    /* Retained bad values must stay intact and must never index the label table. */
    if (argc>1 && !strcmp(argv[1],"invalid-retained")) {
        char number[16],unit[8];
        for (int v=0;v<NBASS;v++) {
            int id=engine_bass_is_wobble(v) ? W_CARRIER_WAVE : BASS303_DRVTYPE;
            pref_t ref=PR(R_ENG,T_303,v,id);
            const x0x_param_t *p=engine_param(T_303,v,id);
            for (int val=0;val<=p->max;val++) {
                pref_value_of(ref,val,number,unit);
                assert(!strcmp(number,p->names[val]));
            }
            pref_value_of(ref,-1,number,unit);
            assert(!strcmp(number,"?"));
            proj.sound.v[T_303][v][id]=255;
            pref_value(ref,number,unit);
            assert(!strcmp(number,"?"));
            assert(proj.sound.v[T_303][v][id]==255);
            ui.part=PART_303A+v;
            set_view(V_PART);
            ui_frame();
        }
    }
    printf("303A/303B entry and requested retained-value checks passed\n");
    return 0;
}
