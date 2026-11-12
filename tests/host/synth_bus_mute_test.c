/* SPDX-License-Identifier: GPL-3.0-only */
/* Real synth render, sequencer and queued SMC dispatch. Removing the bus gate
 * or turning muted sequence steps into rests breaks the audio assertions. */
#define SMC_UI_MAIN synth_mute_unused_ui_main
#include "smc_ui_test.c"

/* Exercise the unchanged physical-fader scanner with real pref_set / mute
 * state. Only the hardware samples and the bank-button bit are substituted. */
static struct { uint32_t buttons,frames; uint8_t fader[4]; } fm1_in;
static uint8_t smk_fbank;
#define SMK_BTN_FADER_BANK 0u
#include "smk_faders.inc"

static void physical_muted_fader(int part,int value)
{
    int position=part_position(part),slot=position%4;
    smk_fbank=(uint8_t)(position/4);
    memset(&fm1_in,0,sizeof fm1_in);fm1_in.frames=300;
    for(int i=0;i<4;i++)fm1_in.fader[i]=10;
    project_busy=1;smk_faders();project_busy=0; /* seed hardware history */
    smk_faders();fm1_in.fader[slot]=(uint8_t)value;smk_faders();
    CHECK(pref_get(PR(R_ENG,T_MIX,part,MX_LEVEL))==value);
}

static uint64_t render_energy(unsigned blocks)
{
    int32_t out[512];
    uint64_t energy = 0;
    while (blocks--) {
        engine_render(out, 256);
        energy = 0; /* final block, after the existing gain glide settles */
        for (unsigned i=0;i<512;i++)
            energy += out[i] < 0 ? (uint64_t)-(int64_t)out[i] : (uint64_t)out[i];
    }
    return energy;
}
static void input_midi(int status,int note,int velocity)
{
    min_q[mi_w++ % MQ] = (uint32_t)(status>>4) | (uint32_t)status<<8 |
        (uint32_t)note<<16 | (uint32_t)velocity<<24;
}
static void queue_mute(int strip)
{
    uint8_t msg[3]={0x90,(uint8_t)(16+strip),127};
    smc_ble_midi(msg,3);
}
static void mute_setup(int bank,int voice)
{
    setup(V_HOME,PART_303A+voice);
    seq_stop(&seq);render_energy(2);
    CHECK(!engine_bass_select_preset(voice,bank,0,&proj.sound));
    render_energy(100);
    CHECK(engine_bass_bank(voice)==bank);
    for(int p=0;p<NPARTS;p++) {
        pref_set(PR(R_ENG,T_MIX,p,MX_LEVEL),p==PART_303A+voice?100:0);
        pref_set(PR(R_ENG,T_MIX,p,MX_REV),0);
        pref_set(PR(R_ENG,T_MIX,p,MX_DLY),0);
    }
    seq.mute=0;render_energy(100);
}
static void held_midi_unmute(int bank,int voice,int pedal)
{
    mute_setup(bank,voice);
    int channel=seq.ch[TRK_BASS0+voice],part=PART_303A+voice;
    int strip=part_position(part);
    input_midi(0x90|channel,60,110);
    uint64_t audible=render_energy(8);CHECK(audible>1000);
    if(pedal) {
        input_midi(0xB0|channel,64,127);render_energy(1);
        input_midi(0x80|channel,60,0);render_energy(1);
    }
    uint32_t before=seq.mute;
    queue_mute(strip);CHECK(seq.mute==before); /* interrupt queue has no UI effects */
    smc_poll();CHECK(seq_part_muted(&seq,part));
    CHECK(render_energy(100)<audible/100);
    /* L8 / #788: edits change the saved target while the output stays muted.
     * No delayed fader application or note retrigger is needed on unmute. */
    fader(strip,40);CHECK(pref_get(PR(R_ENG,T_MIX,part,MX_LEVEL))==40);
    physical_muted_fader(part,25);
    CHECK(seq_part_muted(&seq,part));
    CHECK(render_energy(100)<audible/100);
    message(0x80,16+strip,0); /* rearm the physical mute button */
    queue_mute(strip);smc_poll();CHECK(!seq_part_muted(&seq,part));
    uint32_t input_count=mi_w;
    CHECK(render_energy(2)>1000); /* audible before any next incoming note */
    CHECK(mi_w==input_count);
    CHECK(pref_get(PR(R_ENG,T_MIX,part,MX_LEVEL))==25);
    input_midi(0xB0|channel,64,0);input_midi(0x80|channel,60,0);
    render_energy(100);
}
static void sequence_tie_unmute(int bank,int voice)
{
    mute_setup(bank,voice);
    int part=PART_303A+voice;
    bpart_t *line=&proj.pat[seq.ppat[part]].bass[voice];
    line->len=16;line->step[0]=(bstep_t){60,G_NOTE|BS_SLIDE,110};
    for(int i=1;i<16;i++)line->step[i]=(bstep_t){60,G_TIE,110};
    seq_start(&seq);render_energy(2);
    CHECK(seq.t[part].sounding);
    mute_part(part);
    unsigned guard=0;
    while(eng_step[part]<2 && guard++<1000)render_energy(1);
    CHECK(eng_step[part]>=2);CHECK(seq.t[part].sounding);
    CHECK(render_energy(100)<1000);
    int step=eng_step[part];mute_part(part);
    CHECK(render_energy(1)>1000);
    CHECK(eng_step[part]==step); /* no new step or note triggered the recovery */
    seq_stop(&seq);render_energy(100);
}
int main(void)
{
    for(int bank=FACTORY_BANK_CHICAGO;bank<=FACTORY_BANK_MIAMI;bank++)
        for(int voice=0;voice<NBASS;voice++) {
            held_midi_unmute(bank,voice,0);
            held_midi_unmute(bank,voice,1);
            sequence_tie_unmute(bank,voice);
        }
    printf("synth bus mute: %u checks; queued mute, held/pedal notes, ties and muted fader\n",checks);
    return 0;
}
