# SPDX-License-Identifier: GPL-3.0-only
# Generate whole-app fixture using exact production physical input transport.
from pathlib import Path
import os
root=Path(__file__).resolve().parents[2];out=Path(os.environ['X0X_LIVE_TEST_OUT'])
s=(root/'tests/host/rec_live_test.c').read_text().split('int main(void)')[0]
s=s.replace('#include "../../host/x0x_host.c"',f'#include "{root}/host/x0x_host.c"')
s=s.replace('static uint8_t test_clip_mode, test_clip_busy;','static uint8_t test_clip_mode, test_clip_busy, test_padcal, test_smcui;').replace('uint8_t smk_padcal_on(void) { return 0; }','uint8_t smk_padcal_on(void) { return test_padcal; }').replace('int smc_ui_active(void) { return 0; }','int smc_ui_active(void) { return test_smcui; }')
h=(root/'firmware/hal/smk_input.h').read_text();p=(root/'firmware/src/app/plat_fm1.c').read_text()
def fn(t,start):
 a=t.index(start);return t[a:t.index('\n}',a)+2]
s+='''\n#define SMK_KEY_F3 5u
#define SMK_EXTQ 32u
static struct { uint32_t notes,knotes,pnotes,notes_pressed; uint8_t key_vel[37]; uint64_t keys; } fm1_in;
'''+h[h.index('static volatile uint32_t smk_extq'):h.index('\nstatic uint8_t fm1_led')]
s+='\n'+fn(h,'static void smk__extq(')+'\n'+fn(h,'static void smk__key_event(')
s+='''\nstatic unsigned irq_depth;
static uint32_t irq_save(void){return irq_depth++;}
static void irq_restore(uint32_t f){irq_depth=f;}
'''+fn(p,'uint64_t plat_physical_keys(')+'\n'+fn(p,'int plat_outer_keys(')
s+='\n'+(root/os.environ.get('X0X_LIVE_CASES','tests/host/freeplay_live_keys_cases.inc')).read_text()
(out/'probe.c').write_text(s)
