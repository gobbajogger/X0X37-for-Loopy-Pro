/* SPDX-License-Identifier: GPL-3.0-only */
#include "chicago_smallfx.h"
#include <string.h>
int original_chicago_decimator_init(original_chicago_decimator *s){if(!s)return 0;memset(s,0,sizeof *s);return miami_decimator_adapter_init(&s->adapter);}
int original_chicago_decimator_set(original_chicago_decimator *s,unsigned p,int32_t v){return s&&p<3?miami_decimator_adapter_set(&s->adapter,p+1,v):0;}
void original_chicago_decimator_reset(original_chicago_decimator *s){if(s)(void)miami_decimator_adapter_reset(&s->adapter);}
int original_chicago_decimator_bind_events(original_chicago_decimator *s,const original_chicago_drive_event *p,uint32_t n){
 if(!s||(n&&!p))return 0;
 for(uint32_t i=1;i<n;i++)if(p[i].offset<p[i-1].offset)return 0;
 s->queue=(original_chicago_dyn_queue){p,n,0};return 1;
}
void original_chicago_decimator_direct_event(original_chicago_decimator *s,unsigned type){if(type==5)original_chicago_decimator_reset(s);}
int original_chicago_decimator_process(original_chicago_decimator *s,int16_t *l,int16_t *r,uint32_t n){
 if(!s||(n&&(!l||!r||l==r)))return 0;
 for(uint32_t i=0;i<n;i++){
  while(s->queue.cursor<s->queue.count&&s->queue.events[s->queue.cursor].offset<=i){if(s->queue.events[s->queue.cursor].type==4)original_chicago_decimator_reset(s);s->queue.cursor++;}
  int16_t out[2];(void)miami_decimator_frame(&s->adapter.kernel,i,l[i],r[i],s->adapter.wet_gain,out);l[i]=out[0];r[i]=out[1];
 }return 1;
}
int original_chicago_decimator_advance_zero(original_chicago_decimator *s,uint32_t n){
 if(!s||s->queue.cursor<s->queue.count||s->adapter.wet_gain<0)return 0;
 return miami_decimator_advance_zero(&s->adapter.kernel,n);
}
int32_t original_chicago_bypass_parameter(unsigned p,int32_t v){return (p&~1u)==2?((uint32_t)v&127u):0;}
int original_chicago_bypass_process(int16_t *l,int16_t *r,uint32_t n){return !n||(l&&r&&l!=r);}
