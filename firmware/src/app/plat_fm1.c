/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X platform on the FM-1: plat.h over Felucca's HAL, panel map, USB rings and storage.
 * Part of the unity build (x0x.c), after the HAL, usb.c, storage.c and panel.c. */

/* Full target store ordering for the BREAK pulse publication. */
#include "../../hal/fm1_sync.h"

/* Main-loop CQ producer and ALNK0 consumer share CPU0. Source masking leaves
 * MIDI/clock IRQs alone and never changes the caller's global IRQ state. */
uint32_t plat_cq_lock(void)
{
    return fm1_irq_mask_save(FM1_IRQ_ALNK0);
}
void plat_cq_unlock(uint32_t state)
{
    fm1_irq_mask_restore(FM1_IRQ_ALNK0, state);
}

uint32_t plat_ms(void) { return fm1_ms; }

uint32_t plat_buttons(void)
{
    uint32_t raw = fm1_in.buttons, out = 0, i;
#if BOARD_SMK37
    extern uint8_t smk_clip_mode_on(void);
    uint8_t clip_mode = smk_clip_mode_on();
#if X0X_DEV_PADCAL
    extern uint8_t smk_padcal_on(void);
    if (smk_padcal_on())
        return 0u;   /* the pad calibration has the buttons (main_fm1.c, CLP-79) */
#endif
#endif
    for (i = 0; i < NB; i++)
        if (raw & (1u << panel.btn[i])) {
#if BOARD_SMK37
            /* APC navigation controls must not also trigger X0X page/octave actions. */
            if (clip_mode && panel.btn[i] <= FM1_BTN_OCT_UP)
                continue;
            /* Clip mode: SCALE, CHORD, ARP and NOTE REPEAT belong to the clip keys (main_fm1.c). */
            if (clip_mode && (i == B_SEL || i == B_ENV || i == B_ARP || i == B_LFO))
                continue;
#endif
            out |= 1u << i;
        }
    return out;
}

uint32_t plat_keys(void) { return fm1_in.notes & ((1u << NKEYS) - 1u); }
#if BOARD_SMK37 && X0X_ORIGINAL_SYNTH && X0X_DEV_WOBBLE
int plat_division_pad(unsigned owner)
{
    uint32_t flags = irq_save();
    int hit = smk__division_poll(owner);
    irq_restore(flags);
    return hit;
}
#endif

uint64_t plat_physical_keys(void)
{
#if BOARD_SMK37
    uint32_t flags=irq_save();uint64_t keys=fm1_in.keys;irq_restore(flags);return keys;
#else
    return (uint64_t)plat_keys()<<5;
#endif
}
int plat_outer_keys(plat_outer_event_t *events,uint64_t *keys,int *overflow,
                    uint32_t epoch,int enabled,int octave,int publish)
{
    int n=0;
#if BOARD_SMK37
    uint32_t flags=irq_save();
    *keys=fm1_in.keys;*overflow=smk_extq_overflow;
    if(publish || *overflow)smk_extq_r=smk_extq_w;
    else while(smk_extq_r!=smk_extq_w && n<31) {
        events[n].generation=smk_extq[smk_extq_r][0];events[n++].word=smk_extq[smk_extq_r][1];
        smk_extq_r=(uint8_t)((smk_extq_r+1u)%SMK_EXTQ);
    }
    smk_extq_overflow=0;
    if(publish){smk_ext_epoch=epoch;smk_ext_enabled=(uint8_t)!!enabled;smk_ext_oct=(int8_t)octave;}
    irq_restore(flags);
#else
    (void)events;(void)epoch;(void)enabled;(void)octave;(void)publish;
    *keys=plat_physical_keys();*overflow=0;
#endif
    return n;
}

uint8_t plat_key_velocity(int key)
{
#if BOARD_SMK37
    if ((unsigned)key < NKEYS) {
        uint8_t v = fm1_in.key_vel[SMK_KEY_F3 + key];
        if (v)
            return v;
    }
#else
    (void)key;
#endif
    return 100;
}

int32_t plat_enc(int role)
{
    if (role < 0 || role >= NE)
        return 0;
#if BOARD_SMK37
    extern uint8_t smk_ck_knobs_owned(void);
#if X0X_DEV_PADCAL
    extern uint8_t smk_padcal_on(void);
    if (smk_ck_knobs_owned() || smk_padcal_on())
#else
    if (smk_ck_knobs_owned())
#endif
        return 0;   /* clip mode: knobs 2-8 send CCs (main_fm1.c ck_controls); knob 1 stays MASTER. Pad calibration: K2-K4 */
#endif
    return fm1_enc_take(panel.enc[role]) * panel.dir[role];
}

static volatile uint32_t master_q12 = 2048;
uint32_t plat_master(void) { return master_q12; }

/* the battery: ADC ch3 (its divider), smoothed by the main loop. The stock firmware's thresholds, as
 * melodee and dxsloop use them, give 0-3 bars; there is no charger line, so a USB host means power */
static int32_t batt_raw;
int plat_battery(void)
{
    if (usb.config && !usb.suspended)
        return 4;
    if (!batt_raw)
        return -1;
    return batt_raw >= 591 ? 3 : batt_raw >= 561 ? 2 : batt_raw >= 531 ? 1 : 0;
}

#if BOARD_SMK37
/* SMK-37 Pro LEDs (smk_input.h, lcd.c): the button LEDs are on / off (no dim layer: no glow); the 16 RGB pads
 * show the 16 white keys (the steps; HOME: the patterns), lit bright and dim dim in the colour of the part on
 * screen (app/main_fm1.c smk_pads, after the frame). The keybed has no LEDs: what the FM-1 shows on its black
 * keys (tracks, slices, part mutes) is on the screen only. */
static uint8_t smk_fbank;                           /* FADER BANK: 0 = A, 1 = B (main_fm1.c smk_faders); its LED */
static uint32_t smk_keys_lit, smk_keys_dim;         /* the key picture of the last frame, for the pads */
static void led_pos_init(void) {}
void plat_glow(int on) { (void)on; }
void plat_play_red(int red) { (void)red; }           /* (SEQ PLAY has one LED) */
void plat_keys_dim(uint32_t keys) { smk_keys_dim = keys; }
void plat_leds(uint32_t buttons, uint32_t keys)
{
    uint8_t nl[SMK_NLED] = {0};
    uint32_t i;
    for (i = 0; i < NB; i++)
        if ((buttons >> i) & 1u)
            nl[panel.btn[i]] = 1;
    nl[SMK_BTN_FADER_BANK] = smk_fbank != 0;
    for (i = 0; i < SMK_NLED; i++)
        fm1_led[i] = nl[i];
    smk_keys_lit = keys;
}
#else
/* LEDs: the picture is built off-line and copied one byte per column (Felucca: clearing
 * and relighting would let the 10 kHz scan catch the dark gap and flicker).
 * led_pos[id] = (col << 3) | row from FM1_KEYMAP (was rebuilt into BSS at boot). */
static const uint8_t led_pos[FM1_NKEY] = {
    4, 12, 49, 33, 17, 1, 74, 66, 57, 41, 25, 9, 82, 58, 28, 20, 44, 36, 60, 52, 68, 76, 84,
    3, 11, 19, 27, 35, 43, 51, 59, 67, 75, 83, 2, 10, 18, 26, 34, 50, 42};
static void led_pos_init(void) {}
/* Glow: every button not lit is lit dim, on one scan frame in GLOW_DIV (the HAL's fm1_led_dim) */
#define GLOW_DIV 4u
static uint8_t glow_on = 1;
void plat_glow(int on) { glow_on = (uint8_t)(on != 0); }
/* PLAY has two LEDs: its red at the button's own position, a green at the keyless matrix position
 * column 8, row PA5 (found on the device). Green, unless recording. */
static uint8_t play_red;
void plat_play_red(int red) { play_red = (uint8_t)(red != 0); }
static uint32_t keys_dim;
void plat_keys_dim(uint32_t keys) { keys_dim = keys; }
void plat_leds(uint32_t buttons, uint32_t keys)
{
    uint8_t nl[FM1_NCOL] = {0}, dl[FM1_NCOL] = {0};
    uint32_t i, c;
    for (i = 0; i < NB + NKEYS; i++) {
        uint32_t id = i < NB ? panel.btn[i] : 14u + (i - NB);
        int on = i < NB ? (buttons >> i) & 1u : (keys >> (i - NB)) & 1u;
        uint8_t q = led_pos[id];
        if (i == B_PLAY && !play_red)            /* PLAY's green LED instead (keyless position col 8, row PA5) */
            q = (uint8_t)((8u << 3) | 1u);
        if (q == 0xFF)
            continue;
        if (on)
            nl[q >> 3] |= (uint8_t)(1u << (q & 7u));
        else if (i < NB ? glow_on : (keys_dim >> (i - NB)) & 1u)
            dl[q >> 3] |= (uint8_t)(1u << (q & 7u));
    }
    for (c = 0; c < FM1_NCOL; c++) {
        fm1_led[c] = nl[c];
        fm1_led_dim[c] = dl[c];
    }
    fm1_led_dim_div = glow_on || keys_dim ? GLOW_DIV : 0u;
}
#endif

/* MIDI: Felucca's rings carry 4-byte USB-MIDI event packets, byte 0 = cable / CIN */
uint32_t plat_midi_mark(void)
{
    __asm__ volatile("" ::: "memory");
    return mi_w;
}
int plat_midi_passed(uint32_t mark)
{
    __asm__ volatile("" ::: "memory");
    return (int32_t)(mi_r-mark)>=0;
}
int plat_midi_pending(void)
{
    __asm__ volatile("" ::: "memory");
    return mi_r != mi_w;
}
int plat_midi_in(uint32_t *pkt)
{
    /* Ingress overflow can compact the live ring in TIMER5. Keep its slot
     * read and consume together so an interrupted read cannot skip a note. */
    uint32_t irq = fm1_irq_mask_save(FM1_IRQ_TIMER5);
    int found = mi_r != mi_w;
    if (found) {
        *pkt = midi_in_q[mi_r % MQ];
        mi_r++;
    }
    fm1_irq_mask_restore(FM1_IRQ_TIMER5, irq);
    return found;
}
void plat_midi_out(uint32_t pkt)
{
#if BOARD_SMK37
    extern uint8_t smk_clip_mode_on(void);
    /* Sequencer output must never become APC commands or mix with clip keys. */
    if (smk_clip_mode_on())
        return;
#endif
    midi_out_event(pkt);
#if X0X_BLE
    ble_out(pkt);                               /* the same, to a Bluetooth MIDI connection */
#endif
}
/* A pulse must not use priority replacement: an Off replacing an older CC
 * could otherwise precede its newly appended On. Reserve two slots or drop both. */
int plat_midi_pulse(uint8_t channel, uint8_t note, uint8_t velocity)
{
#if BOARD_SMK37
    extern uint8_t smk_clip_mode_on(void);
    if (smk_clip_mode_on()) return 0;
#endif
    uint32_t on = 9u | ((uint32_t)(0x90u | channel) << 8) |
        ((uint32_t)note << 16) | ((uint32_t)velocity << 24);
    uint32_t off = 8u | ((uint32_t)(0x80u | channel) << 8) | ((uint32_t)note << 16);
    int sent = 0;
    /* Called by CPU0 only. Exclude its audio producer and TIMER5 USB/BLE
     * consumer/link transitions, preserving source enables and global IRQ state.
     * The generic CLI/STI flash helpers are intentionally not used. */
    uint32_t audio = fm1_irq_mask_save(FM1_IRQ_ALNK0);
    uint32_t transport = fm1_irq_mask_save(FM1_IRQ_TIMER5);
    if (usb.config && mo_w - mo_r <= MQ - 2u) {
        midi_out_q[mo_w % MQ] = on;
        midi_out_q[(mo_w + 1u) % MQ] = off;
        fm1_data_sync(); mo_w += 2u;
        sent |= 1;
    }
#if X0X_BLE
    if (!ble_central_role && !smc_ble_role_get() && ble_want && ble_host_on &&
        bleh_notifying(&ble_r.host.h) && ble_r.ow - ble_r.orr <= BLE_OQ - 2u) {
        ble_oq[ble_r.ow % BLE_OQ] = on;
        ble_oq[(ble_r.ow + 1u) % BLE_OQ] = off;
        fm1_data_sync(); ble_r.ow += 2u;
        sent |= 2;
    }
#endif
    fm1_irq_mask_restore(FM1_IRQ_TIMER5, transport);
    fm1_irq_mask_restore(FM1_IRQ_ALNK0, audio);
    return sent;
}
#if !X0X_BLE
int plat_ble_state(void) { return PLAT_BLE_NOIMG; }
const char *plat_ble_name(void) { return ""; }
void plat_ble_peers(plat_ble_peers_t *out) { __builtin_memset(out,0,sizeof *out); out->state=PLAT_BLE_NOIMG; }
void plat_ble_set(int on) { (void)on; }
int plat_ble_smc_send(const uint8_t *msg, uint8_t len) { (void)msg; (void)len; return 0; }
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *msg, uint8_t len) { (void)slot; (void)msg; (void)len; return 0; }
#endif

int plat_store_load(uint32_t obj, void *dst, uint32_t max) { return flash_ok ? st_load(obj, dst, max) : -1; }
int plat_store_save(uint32_t obj, const void *src, uint32_t len) { return flash_ok ? st_save(obj, src, len) : -9; }

int plat_slot_zones(int k) { return (k >= 0 && k < SMP_USER_SLOTS) ? usr_nz[k] : 0; }

const uint8_t *plat_slot(int k, int z, uint32_t *nsamples, uint32_t *rate, char name[9])
{
    const smp_user_hdr_t *h;
    uint32_t i, rate_q16;
    if (k < 0 || k >= SMP_USER_SLOTS || z < 0 || z >= usr_nz[k])
        return 0;
    h = (const smp_user_hdr_t *)smp_user_xip((uint32_t)k);
    for (i = 0; i < 8u; i++)
        name[i] = h->name[i] >= 32 && h->name[i] < 127 ? h->name[i] : 0;
    name[8] = 0;
    *nsamples = h->zone[z].n;
    rate_q16 = h->zone[z].rate;
    /* Split Q16 before multiplying: 88.2 kHz would overflow a uint32_t product. */
    *rate = (rate_q16 >> 16) * 44100u +
            (((rate_q16 & 0xFFFFu) * 44100u + 32768u) >> 16);
    return smp_user_xip((uint32_t)k) + SMP_USER_DATA + h->zone[z].off;
}

static volatile uint32_t audio_cpu_pct, audio_xruns, audio_peak_pct;
uint32_t plat_cpu_pct(void) { return audio_cpu_pct; }

int plat_cpu2_set(int on)
{
    int rc = 0;
    if (!on) {
        fm1_cpu1_hold();
        return 0;
    }
    {
        int locked = fm1_guard_top_locked();            /* at boot the top is not locked yet: leave it so */
        if (locked)
            fm1_guard_unlock_top();                     /* its entry word is in the protected top of RAM */
        fm1_guard_pc_open();                            /* it starts in the chip's ROM */
        rc = fm1_cpu1_start();
        fm1_guard_enable(FM1_GUARD_PC);
        if (locked)
            fm1_guard_lock_top();
    }
    return rc;
}
int plat_cpu2_on(void) { return fm1_c1_on; }
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg) { return fm1_cpu1_run(fn, arg); }
int plat_cpu2_wait(void) { return fm1_cpu1_wait(); }
int plat_cpu2_gave_up(void) { return fm1_c1_gave_up; }
uint32_t plat_cpu2_bad(void) { return fm1_c1_mb.bad; }
uint32_t plat_xruns(void) { return audio_xruns; }

static int perf_cyc_ok, perf_stalls_on;
void plat_perf_init(void) { perf_cyc_ok = fm1_perf_cycles_run(); }
uint32_t plat_cycles(void) { return perf_cyc_ok ? fm1_perf_cycles() : fm1_ticks(); }
int plat_cycles_cpu(void) { return perf_cyc_ok; }
uint32_t plat_cycles_hz(void) { return 24000000u; }   /* the timer; the CPU's rate is measured */
uint32_t plat_ticks24(void) { return fm1_ticks(); }
/* the interrupts' stack (fm1_guard.h): filled with a mark before any interrupt runs (irq_stack_mark,
 * fm1_cstart), so the deepest point reached is the lowest word no longer marked */
#define IRQ_STACK_MARK 0x49525153u                  /* "IRQS" */
static void irq_stack_mark(void)
{
    uint32_t *p = (uint32_t *)(void *)_sstack_lo, *top = (uint32_t *)(void *)_sstack_top - 16;
    uint32_t here = (uint32_t)(uintptr_t)&p;        /* never mark below a live frame of our own */
    if (here > (uint32_t)(uintptr_t)p && here < (uint32_t)(uintptr_t)top)
        top = (uint32_t *)(uintptr_t)((here - 256u) & ~3u);
    for (; p < top; p++)
        *p = IRQ_STACK_MARK;
}
uint32_t plat_irq_stack_size(void) { return (uint32_t)(_sstack_top - _sstack_lo); }
uint32_t plat_irq_stack_used(void)
{
    const uint32_t *p = (const uint32_t *)(const void *)_sstack_lo, *top = (const uint32_t *)(const void *)_sstack_top;
    while (p < top && *p == IRQ_STACK_MARK)
        p++;
    return (uint32_t)((const char *)top - (const char *)p);
}

uint32_t plat_cpu_peak_pct(void)
{
    uint32_t p = audio_peak_pct;
    audio_peak_pct = 0;
    return p;
}
int plat_stalls(uint32_t s[3])
{
    if (!perf_stalls_on)
        return -1;
    fm1_perf_stalls(s);
    return 0;
}
void plat_stalls_enable(int on)
{
    perf_stalls_on = on;
    fm1_perf_stalls_enable(on);
}
