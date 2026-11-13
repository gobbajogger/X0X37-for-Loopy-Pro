/* SPDX-License-Identifier: GPL-3.0-only */
/* Free Play key-to-sound latency (owner report on 0.0.20: a huge delay between a
 * key and its sound on Chicago and Miami).
 *
 * A device-timeline model driven by the production code: the key edge goes
 * smk__key_event -> main loop input() -> ui live route -> engine live-key FIFO ->
 * the ALNK0 half render (2 x engine_render(256)) -> the DAC one half later.
 * The main loop is main_fm1.c's: ui_frame() when 16 ms have passed, else
 * ui_input_only(). A ui_frame costs its SPI time: the pixels it sends to the
 * ST7789 at the SMK-37's 12 MHz (16 bits each) plus a fixed draw cost. A key
 * that lands during a frame waits for that frame to end. Latency = the time
 * the first non-silent sample reaches the DAC minus the key time. */
static unsigned checks;
#define T(c) do{checks++;if(!(c)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#c);exit(1);}}while(0)
#define HALF 512u
#define HALF_US 11610.0                     /* 512 frames at 44.1 kHz */
#define SPI_US_PER_PX (16.0/12.0)           /* LCD_BAUD 4: 12 MHz */
#define FRAME_CPU_US 1500.0                 /* canvas drawing, hashing (not SPI) */
#define INPUT_US 60.0
static int32_t half_out[2u*HALF];
static double t_us,next_irq_us,last_frame_us;
static double key_us;int key_waiting;double lat_sum,lat_worst;unsigned lat_n;
static double frame_worst_us;
static unsigned saves,frames_slow,frames_n;static double frames_sum;
/* the note's own rise: output after the key, to find 50 % of its first-150 ms peak */
#define ENV_N 8192u
static int32_t env_abs[ENV_N];static double env_t0,env_h0;static unsigned env_n;static int env_on;
static double rise_sum,rise_worst;unsigned rise_n;
static void physical(unsigned p,unsigned velocity)
{
    smk__key_event(p,velocity);held_keys=fm1_in.notes;
    for(unsigned k=0;k<NKEYS;k++)key_velocity[k]=fm1_in.key_vel[k+5];
}
static void irq(void)                       /* fm1_alnk0_irq: one half, heard one half later */
{
    engine_render(half_out,256);engine_render(half_out+512,256);
    if(env_on && next_irq_us>=env_t0) {      /* halves rendered after the key */
        if(!env_n)env_h0=next_irq_us+HALF_US;
        for(unsigned i=0;i<HALF && env_n<ENV_N;i++){int32_t a=half_out[2u*i];env_abs[env_n++]=a<0?-a:a;}
    }
    if(key_waiting) for(unsigned i=0;i<HALF;i++) if(half_out[2u*i]||half_out[2u*i+1u]) {
        double heard=next_irq_us+HALF_US+i*1e6/44100.0,l=heard-key_us;
        lat_sum+=l;lat_n++;if(l>lat_worst)lat_worst=l;key_waiting=0;break;
    }
    next_irq_us+=HALF_US;
}
static void advance(double us)              /* the main loop is busy for us; the ISR preempts it */
{
    t_us+=us;
    while(next_irq_us<=t_us)irq();
    now_ms=(uint32_t)(t_us/1000.0);
}
/* the next key edge, applied when the device time reaches it (the key ISR) */
static double ev_us=1e300;static unsigned ev_p,ev_vel;static int ev_on;
static void ev_apply(double now)
{
    if(!ev_on || now<ev_us)return;
    ev_on=0;physical(ev_p,ev_vel);
}
static void loop_once(void)
{
    ev_apply(t_us);
    if(t_us-last_frame_us>=16000.0) {
        last_frame_us=t_us;uint64_t px0=blit_px;double t0=t_us;
        ui_frame();
        double cost=FRAME_CPU_US+(double)(blit_px-px0)*SPI_US_PER_PX;
        cost-=t_us-t0;if(cost<0)cost=0;
        if(cost+(t_us-t0)>frame_worst_us)frame_worst_us=cost+(t_us-t0);frames_n++;frames_sum+=cost+(t_us-t0);if(cost+(t_us-t0)>16000)frames_slow++;
        advance(cost);
    } else { ui_input_only(); advance(INPUT_US); }
}
static void run_until(double us){while(t_us<us){uint32_t st=ui.saved_t;loop_once();if(ui.saved_t!=st)saves++;}}
static void reset_stats(void){lat_sum=lat_worst=0;lat_n=0;frame_worst_us=0;rise_sum=rise_worst=0;rise_n=0;saves=frames_slow=frames_n=0;frames_sum=0;}
static void rise_done(void)
{
    int32_t pk=0;unsigned n=env_n<6615u?env_n:6615u;
    for(unsigned i=0;i<n;i++)if(env_abs[i]>pk)pk=env_abs[i];
    for(unsigned i=0;i<n;i++)if(pk && env_abs[i]>=pk/2){double l=(env_h0-env_t0)/1000.0+i*1000.0/44100.0;rise_sum+=l;rise_n++;if(l>rise_worst)rise_worst=l;break;}
    env_on=0;
}
static void fresh(int bank,int view)
{
    for(unsigned p=0;p<37;p++)physical(p,0);
    ui.freeplay[0]=ui.freeplay[1]=0;ui.rec=0;ui.overlay=O_NONE;ui.help=0;
    test_clip_mode=test_padcal=test_smcui=0;held_btn=ui.btn=0;ui.held_step=-1;
    seq_stop(&seq);run_until(t_us+50000);engine_bass_off(0);engine_bass_off(1);
    T(!engine_bass_select_preset(0,bank,0,&proj.sound));run_until(t_us+300000);
    ui.part=PART_303A;set_view(view);build_pages();ui.oct=0;
    ui.freeplay[0]=1;run_until(t_us+1000000);
}
/* A phrase: key presses at phases spread over the frame and half grids, each
 * held 180 ms, 70 ms apart, one legato overlap in four (the owner plays lines). */
static void phrase(void)
{
    static const unsigned keys[8]={5,9,12,16,19,24,28,31};
    double t=t_us+3333.0;
    for(int n=0;n<64;n++) {
        unsigned p=keys[n&7];
        key_us=t;key_waiting=1;env_t0=t;env_n=0;env_on=1;
        ev_us=t;ev_p=p;ev_vel=100;ev_on=1;
        t+=180000.0+((n*7919)%13000);
        run_until(t);
        if(key_waiting){lat_sum+=1e6;lat_n++;lat_worst=1e6;key_waiting=0;}
        rise_done();ev_us=t;ev_p=p;ev_vel=0;ev_on=1;
        t+=70000.0+((n*104729)%9000);
        run_until(t);
    }
}
int main(int argc,char **argv)
{
    (void)argc;(void)argv;boot();tick(300);
    t_us=now_ms*1000.0;next_irq_us=t_us;last_frame_us=t_us;
    /* audio alone: 11.6..23.2 ms (one half to wait, one half to play) plus the attack */
    double limit=getenv("X0X_LAT_LIMIT_MS")?atof(getenv("X0X_LAT_LIMIT_MS")):45.0;
    double mean_limit=getenv("X0X_LAT_MEAN_MS")?atof(getenv("X0X_LAT_MEAN_MS")):25.0;
    static const char *name[2]={"Chicago","Miami"};
    static const int views[2]={V_PART,V_SEQ};static const char *vname[2]={"EDIT","SEQ"};
    for(int bank=FACTORY_BANK_CHICAGO;bank<=FACTORY_BANK_MIAMI;bank++)
    for(int v=0;v<2;v++) {
        fresh(bank,views[v]);reset_stats();phrase();
        printf("LAT %-7s %-4s: mean %5.1f ms worst %5.1f ms (n=%u); slowest UI frame %5.1f ms; key to 50%% of the note's peak %5.1f ms mean %5.1f worst\n",
            name[bank-FACTORY_BANK_CHICAGO],vname[v],lat_sum/lat_n/1000.0,lat_worst/1000.0,lat_n,frame_worst_us/1000.0,rise_n?rise_sum/rise_n:0,rise_worst);
        printf("    UI frames %u, mean %.1f ms, over 16 ms %u; autosaves during play %u\n",frames_n,frames_sum/frames_n/1000.0,frames_slow,saves);
        {   /* the panel after partial sends equals a full repaint of the same frame */
            static uint16_t shown[240*240];uint32_t keep=last_frame_us;(void)keep;
            memcpy(shown,fb,sizeof fb);
            for(unsigned r=0;r<4;r++)blit_hash[r]=0;
            ui_frame();T(!memcmp(shown,fb,sizeof fb));
        }
        T(lat_worst/1000.0<limit);T(lat_sum/lat_n/1000.0<mean_limit);
    }
    printf("PASS %u checks: free play key-to-sound mean under %.0f ms, worst under %.0f ms\n",checks,mean_limit,limit);
    return 0;
}
