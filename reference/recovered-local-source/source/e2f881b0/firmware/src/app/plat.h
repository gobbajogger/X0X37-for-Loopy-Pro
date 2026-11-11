/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X platform interface: everything the app needs from the hardware.
 *
 * The device implementation (plat_fm1.c) wraps Felucca's HAL and panel map; the
 * host implementation (host/plat_host.c) is a simulator fed by a script, which is
 * what lets the whole app — UI, sequencer, engines — run and be tested on a Mac.
 * Drawing goes through Felucca's lcd_fill / lcd_blit (lcd.c on the device, a
 * framebuffer on the host) and gfx.c on top. */
#pragma once
#include <stdint.h>

/* big zero-initialised buffers go in the .pool RAM region on the device */
#ifdef X0X_HOST
#define X0X_POOL
#else
#define X0X_POOL __attribute__((section(".pool")))
#endif

/* CQ replacement: exclude only its audio-ISR consumer, preserving an already
 * disabled source. Host app simulation is single-threaded; interleaving tests
 * supply a source-mask model when extracting the production queue. */
#ifdef X0X_HOST
static inline uint32_t plat_cq_lock(void) { return 0; }
static inline void plat_cq_unlock(uint32_t state) { (void)state; }
#else
uint32_t plat_cq_lock(void);
void plat_cq_unlock(uint32_t state);
#endif

/* panel controls, by printed label (Felucca's panel.c map turns these into matrix ids; Felucca
 * calls the second button SCL, the panel prints SEL) */
enum { B_FX, B_SEL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SAVE, B_ARP, B_SEQ, B_PLAY, B_REC,
       B_OCTDN, B_OCTUP, NB };
enum { EN_SELECT, EN_ALGO, EN_PRESET, EN_K1, EN_K2, EN_K3, EN_K4, NE };

#define NKEYS 27                       /* F3 .. G5 */

uint32_t plat_ms(void);
uint32_t plat_buttons(void);           /* held: bit B_* */
uint32_t plat_keys(void);              /* held: bit k = key k (0 = F3) */
uint8_t plat_key_velocity(int key);    /* last strike, 1..127; 100 without velocity sensing */
int32_t plat_enc(int role);            /* detents since the last call, + = clockwise; EN_* */
uint32_t plat_master(void);            /* MASTER pot, 0..4096 gain Q12 */
int plat_battery(void);                /* 0..3 bars, 4 = powered by a USB host, -1 = not read yet */
void plat_leds(uint32_t buttons, uint32_t keys);   /* lit: bit B_*, bit key */
void plat_glow(int on);                /* the unlit buttons glow dimly, readable in the dark (call before plat_leds) */
void plat_keys_dim(uint32_t keys);     /* these keys light dim (a second level; call before plat_leds) */
void plat_play_red(int red);           /* PLAY lights green; red (its other LED) while recording (before plat_leds) */

/* USB MIDI, 4-byte USB-MIDI event packets (cable 0) */
uint32_t plat_midi_mark(void);        /* accepted input producer watermark, non-consuming */
int plat_midi_passed(uint32_t mark);  /* consumer reached/passed mark; <2^31 outstanding */
int plat_midi_pending(void);          /* non-consuming shared USB/BLE/TRS input ring query */
int plat_midi_in(uint32_t *pkt);       /* 1 = got one (called from the audio ISR) */
#define PLAT_MIDI_SRC(pkt) (((pkt) >> 4) & 3u)   /* the input of a MIDI in packet: 0 USB, 1 Bluetooth, 2 TRS */
#define PLAT_MIDI_LINK_END(src) ((uint32_t)(src) << 4)   /* CIN 0, no message: the link of input src ended (MID-15, BLE-16) */
int plat_midi_pulse(uint8_t channel, uint8_t note, uint8_t velocity); /* CPU0 only: atomic matched pair or drop both */
void plat_midi_out(uint32_t pkt);      /* queue one (audio ISR or main loop) */

/* persistent storage (Felucca storage.c objects: A/B sectors, CRC, torn-write safe) */
int plat_store_load(uint32_t obj, void *dst, uint32_t max);   /* bytes loaded, < 0 = none / bad */
int plat_store_save(uint32_t obj, const void *src, uint32_t len);   /* 0 = ok */
#define PLAT_STORE_MAX 3840u           /* bytes per object */
enum { OBJ_SET, OBJ_SOUND, OBJ_PAT0, OBJ_PAT1, OBJ_PANEL, OBJ_SONG, OBJ_MOTION, OBJ_NOBJ };
/* flash: OBJ_SET 0xFC000; SOUND / PAT0 / PAT1 / SONG 0x97000.. (2 sectors each); OBJ_PANEL 0xDC000,
 * OBJ_MOTION 0xDE000 (all inside Felucca's data area, 0x97000..0xDFFFF) */

/* user sample slots (break loops), read in place; 0 = empty slot.
 * Two slots. slots.c SMP_USER_SLOTS is the same count. */
#define PLAT_NSLOTS 2
/* every zone of a slot is one loop (tools/upload_breaks.py packs several bars per slot) */
#define PLAT_SLOT_ZONES 16
int plat_slot_zones(int k);            /* loops in slot k, 0 = empty */
/* slot k, zone z: IMA ADPCM from predictor 0 / index 0, *nsamples, *rate in Hz; name = the slot's */
const uint8_t *plat_slot(int k, int z, uint32_t *nsamples, uint32_t *rate, char name[9]);

/* audio load in % of the render budget, and xruns since boot (device: from the ISR) */
uint32_t plat_cpu_pct(void);
/* the second core: the 909 and 808 render there while the first renders the rest (engine.c) */
int plat_cpu2_set(int on);             /* main loop: 0 = done; -1 = it did not start (one core) */
int plat_cpu2_on(void);
int plat_cpu2_run(void (*fn)(uint32_t), uint32_t arg);   /* ISR: 1 = the second core has it */
int plat_cpu2_wait(void);              /* ISR: 0 = done; -1 = it failed (and is off): do the job here */
int plat_cpu2_gave_up(void);           /* why it is off: 0 no, 1 it read a wrong job (fm1-x0x#10), 2 one did not finish */
uint32_t plat_cpu2_bad(void);          /* wrong jobs it saw (0 on a unit that runs both cores) */
uint32_t plat_xruns(void);

/* Bluetooth MIDI (SMK-37, src/ble.c): the state for the GLOBAL menu, the name the unit advertises
 * ("X0X-XXXX", "" without a controller image), and the switch (ON takes effect at once only when
 * it was ON at power-on; OFF drops the link and stops advertising) */
enum { PLAT_BLE_NOIMG, PLAT_BLE_OFF, PLAT_BLE_RESTART, PLAT_BLE_STARTING, PLAT_BLE_READY, PLAT_BLE_CONNECTED,
       PLAT_BLE_FAILED, PLAT_BLE_BADIMG };
int plat_ble_state(void);
const char *plat_ble_name(void);
#define PLAT_BLE_PEER_CAP 2u
#define PLAT_BLE_PEER_NAME_MAX 20u
typedef struct { unsigned count; int state, central; char name[PLAT_BLE_PEER_CAP][PLAT_BLE_PEER_NAME_MAX+1u]; } plat_ble_peers_t;
void plat_ble_peers(plat_ble_peers_t *out);
void plat_ble_set(int on);
/* SMC-Mixer central LED/pickup feedback; 1 queued, 0 disconnected/full. */
int plat_ble_smc_send(const uint8_t *msg, uint8_t len);
int plat_ble_smc_send_slot(unsigned slot, const uint8_t *msg, uint8_t len);

/* performance (the PERF screen; firmware/hal/fm1_perf.h): a counter for timing stages, the CPU's
 * clock cycles when its counter runs, else the 24 MHz timer; which one it is; the 24 MHz timer;
 * the highest half-buffer load since the last call, %; the stall counters (fetch, read, write:
 * 0 = read, -1 = off), and switching them on (experimental: see fm1_perf.h) */
void plat_perf_init(void);
uint32_t plat_cycles(void);
int plat_cycles_cpu(void);
uint32_t plat_cycles_hz(void);           /* the counter's rate when it is not the CPU's */
uint32_t plat_ticks24(void);
uint32_t plat_cpu_peak_pct(void);
uint32_t plat_irq_stack_used(void);      /* the deepest the interrupts' stack has gone, bytes (0: unknown) */
uint32_t plat_irq_stack_size(void);
int plat_stalls(uint32_t s[3]);
void plat_stalls_enable(int on);
