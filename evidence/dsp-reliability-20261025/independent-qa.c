#include <stdio.h>
#include <string.h>
#include "bass303.h"
static int fails, checks;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); fails++; } } while(0)
static void run(bass303_t *b,int n){ float o[128]; while(n){int m=n>128?128:n; bass303_render(b,o,m); n-=m;} }
static void init(bass303_t *b,int lite,int accent,int sustain){bass303_init(b);bass303_set_lite(b,lite);bass303_set(b,BASS303_FENV,1);bass303_set(b,BASS303_FATTACK,127);bass303_set(b,BASS303_DECAY,127);bass303_set(b,BASS303_FSUSTAIN,sustain);bass303_set(b,BASS303_FRELEASE,127);bass303_note_on(b,45,accent,0);}
int main(void){int l,a,s;for(l=0;l<2;l++){for(a=0;a<2;a++){for(s=0;s<3;s++){
 int sustain=s==0?0:s==1?64:127; bass303_t b,c; float old;
 init(&b,l,a,sustain);run(&b,128);CHECK(b.fenv_stage==1&&b.fenv_y>0&&b.fenv_y<1);old=b.fenv_y;c=b;
 bass303_set(&b,BASS303_DECAY,0);run(&b,128);run(&c,128);CHECK(b.fenv_stage==1&&b.fenv_y>old&&b.fenv_y==c.fenv_y);
 bass303_set(&b,BASS303_FATTACK,0);run(&b,128);CHECK(b.fenv_stage>=2);
 if(sustain<127){CHECK(b.fenv_stage==2);old=b.fenv_y;run(&b,128);CHECK(b.fenv_y<old);c=b;bass303_set(&b,BASS303_DECAY,127);run(&b,128);run(&c,128);CHECK(b.fenv_y>c.fenv_y);}
 run(&b,176400);CHECK(b.fenv_stage==3&&b.fenv_y==b.fsustain);old=b.fenv_y;bass303_note_on(&b,57,!a,1);CHECK(b.fenv_stage==3&&b.fenv_y==old);run(&b,128);CHECK(b.fenv_stage==3&&b.fenv_y==old);
 bass303_set(&b,BASS303_DECAY,0);run(&b,128);CHECK(b.fenv_stage==3&&b.fenv_y==b.fsustain);
 bass303_set(&b,BASS303_FSUSTAIN,127-sustain);run(&b,128);CHECK(b.fenv_stage==3&&b.fenv_y==b.fsustain);
 c=b;bass303_note_off(&b);bass303_note_off(&c);bass303_set(&b,BASS303_DECAY,127);run(&b,128);run(&c,128);CHECK(b.fenv_stage==0&&b.fenv_y==c.fenv_y);CHECK(b.fenv_y<=b.fsustain);run(&b,176400);CHECK(b.fenv_y==0&&b.idle);
 bass303_set(&b,BASS303_FATTACK,127);bass303_note_on(&b,45,a,0);CHECK(b.fenv_stage==1&&b.fenv_y==0&&b.fenv_samples==0);run(&b,128);CHECK(b.fenv_y>0&&b.fenv_y<1);
 }}}
printf("independent transitions: %d checks, %d failures\n",checks,fails);return fails!=0;}
