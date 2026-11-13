/* SPDX-License-Identifier: GPL-3.0-only */
/* Production project + UI + engine admission, not a replacement serializer. */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
static unsigned checks;
#define CHECK(x) do {checks++;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(void)
{
    load_force=0;boot();engine_project_hold(1);project_defaults();
    engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    ui.part=PART_303A;ui.wpos[0]=0;ui.spage=0;seq.ppat[PART_303A]=0;
    uint8_t chord[]={60,64,67},notes[6];
    proj.pat[0].bass[0].step[0]=(bstep_t){60,G_NOTE,100};
    CHECK(held_chord_set(proj.arr.lane,0,0,0,chord,3));
    project_capture_stage();CHECK(project_validate_stage()==0);
    CHECK(stream_format_of(&proj)==15u);CHECK(!stream_parts_ok(proj.pat,&proj.arr,13u));
    CHECK(project_save()==0);CHECK(project_load_stage()==0);
    CHECK(held_chord_get(project_stage.arr.lane,0,0,0,60,notes)==3&&!memcmp(notes,chord,3));
    project_stage.arr.lane[1]=project_stage.arr.lane[0];CHECK(project_validate_stage()!=0);
    ui.held_step=0;held_note_by(12);CHECK(proj.pat[0].bass[0].step[0].note==72);
    CHECK(held_chord_get(proj.arr.lane,0,0,0,72,notes)==3&&notes[1]==76&&notes[2]==79);
    bass_write(0,62,G_NOTE);CHECK(held_chord_get(proj.arr.lane,0,0,0,62,notes)==1);CHECK(motion_count(proj.arr.lane)==0);
    CHECK(held_chord_set(proj.arr.lane,0,0,0,chord,3));
    bass_step_key(0,1);bass_step_key(0,0);CHECK(bstep_gate(&proj.pat[0].bass[0].step[0])==G_REST);CHECK(motion_count(proj.arr.lane)==0);
    /* Stopped held-key capture groups actual pitches into one cell. */
    ui.wpos[0]=0;kbd_arp_n[0]=1;kbd_arp_note[0][0]=60;bass_rec_step[0]=0;
    bass_write_held_chord(0);CHECK(ui.wpos[0]==1);
    kbd_arp_n[0]=3;kbd_arp_note[0][1]=67;kbd_arp_note[0][2]=64;
    bass_write_held_chord(0);CHECK(ui.wpos[0]==1);CHECK(held_chord_get(proj.arr.lane,0,0,0,60,notes)==3);
    /* Combined admission accounts for a new NOTE LOCK consuming one lane. */
    memset(proj.arr.lane,0,sizeof proj.arr.lane);
    for(int k=1;k<NLANE;k++)proj.arr.lane[k].used=1;
    engine_note_lock_capture(1);CHECK(!seq_bass_chord_set(&seq,0,0,0,chord,-2));
    CHECK(!proj.arr.lane[0].used);engine_note_lock_capture(0);
    CHECK(seq_bass_chord_set(&seq,0,0,0,chord,-2));
    printf("PASS held-chord production storage/UI/admission: %u checks\n",checks);return 0;
}
