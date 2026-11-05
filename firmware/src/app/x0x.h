/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X app: the project (everything that is saved) and the app-level entry points.
 * Included by the unity build after gfx.c, so drawing helpers are in scope. */
#pragma once
#include <stdint.h>
#ifndef X0X_ORIGINAL_SYNTH
#define X0X_ORIGINAL_SYNTH 1
#endif
#if X0X_ORIGINAL_SYNTH != 1
#error Only X0X_ORIGINAL_SYNTH=1 is supported
#endif
#include "plat.h"
#include "engine.h"
#include "../seq/pattern.h"
#include "../seq/sequencer.h"
#include "../seq/tb3po.h"
#include "../seq/motion.h"
#include "../dsp/breaks.h"
#include "../dsp/fxbus.h"            /* FX_* param indices (the send pages) */
#include "factory_banks.h"           /* Chicago / Miami factory metadata */

#ifndef X0X_VERSION                     /* a release build passes its numeric X.Y.Z (tools/build.py --release) */
#define X0X_VERSION "DEV"
#endif
#ifndef X0X_RELEASE_LABEL              /* display only, e.g. 0.1.3_openbeta; not the package identity */
#define X0X_RELEASE_LABEL ""
#endif
#ifndef X0X_DEV_PADCAL                  /* the pad calibration (BEHAVIOR.md CLP-79): 1 dev build, 0 release build */
#define X0X_DEV_PADCAL 1                /* (tools/build.py passes X0X_DEV_PADCAL=0 or 1 from the environment) */
#endif
#define PROJ_MAGIC 0x50305830u           /* "0X0P" */
#define PROJ_FORMAT 1u
#define PROJ_STEREO 2u
#if X0X_ORIGINAL_SYNTH
#define PROJ_SND_REV 13u
#elif X0X_BASS_TAGGED_SOUND
#define PROJ_SND_REV 11u
#else
#define PROJ_SND_REV 10u
#endif
/* Revision13 stores schema2 cells with55 Chicago programs and seven FX choices.
 * Revision12 stored diagnostic81-program OS cells with OCH1/OMI1 tags.
 * The original profile accepts no earlier engine or project revision.
 * Revision11 appended two explicit engine/product-schema identities. */
/* 1: the 909 SD has a DECAY pot (index 3; LEVEL.. moved up one);
                                          * 2: the master's one-knob COMP and FILTER, the delay's TAPE;
                                          * 3: the 909 BD has sends (REV, DLY at 8, 9; PAN moved to 10);
                                          * 4: the 808 BD too (REV, DLY at 7, 8; PAN moved to 9);
                                          * 5: sound cells widened 16→NPARAMS_MAX; 303 AmpMod (Env/Gate) at 16;
                                          * 6: 303 Wave continuous morph (old 1 → 127); Gnaw pot at 17;
                                          * 7: NPARAMS_MAX 20→24; 303 Bite at 18; amp Env A/D/S/R at 19..22;
                                          * 8: NPARAMS_MAX 24→28; Chicago Arp Switch/Mode/Range/Speed at 23..26;
                                          * 9: NPARAMS_MAX 28→32; Chicago SEND FX Type/Edit1/Edit2 at 27..29
                                          *    (shared Mix/fxbus bridge — not insert FX; see chicago_fx.h);
                                          * 10: Chicago pitch bend + damper at 30..31 (fills NPARAMS_MAX;
                                          *     live ±2 st bend; damper sustains live/MIDI only) */
/* settings_t.bt, bits 1-2: GLO > MIDI > CLOCK SOURCE (SEQ_CLK_*). Bit 3: bits 1-2 hold it (a75 and later). Without
 * bit 3 (a74 and older), project_load reads the a74 byte at offset 38 (padding then; ck_x1 in the clip keys). */
#define PROJ_BT_CLK_MASK 6u
#define PROJ_BT_CLK_IN 8u
#define PROJ_BT_SMC_CENTRAL 16u  /* connect to an SMC-Mixer instead of advertising at next boot */
#define PROJ_BT_SMC_USER 32u     /* legacy reserved bit: accepted on import, never an active profile */
#define PROJ_DEVICE_BT (1u | PROJ_BT_SMC_CENTRAL)
#define PROJ_CLK_SRC(bt) ((uint8_t)((bt) >> 1 & 3u))
#define PROJ_A74_CLK_OFF 38u

typedef struct {
    uint32_t magic, format;
    uint8_t palette, keyled, clk_out, notes_out;   /* keyled: 0 off, 1 keys and the buttons' glow, 2 keys only */
    uint16_t bpm_x10;
    uint8_t accent_q7, keysound;         /* black drum keys sound: 0 = only when stopped or recording, 1 = always */
    uint8_t autosave_off;                /* 0: save by itself while stopped and idle (ui.c autosave) */
    uint8_t rsv_bright;                  /* was the screen's brightness (removed: always full); unused */
    uint8_t stereo;                      /* the pans the sound has: 0 none (saved mono), 1 the parts', 2 the
                                          * drum voices' too (PROJ_STEREO); a load centres what is missing */
    uint8_t snd_rev;                     /* the sound's layout (PROJ_SND_REV): 0 = before the 909 SD's DECAY */
    uint8_t midi_ch[5];                  /* each part's MIDI channel (in and out): 0 = its default (an older
                                          * project), 1-16, 17 = OFF */
    uint8_t out_loud;                    /* 1: the output 6 dB hotter (the codec's full scale) */
    uint8_t usb_full;                    /* 1: USB audio at full level, the MASTER knob for the FM-1's own outputs */
    uint8_t bt;                          /* bit 0: Bluetooth MIDI on (SMK-37; 0 = off, also in older objects);
                                          * bits 1-3: CLOCK SOURCE (PROJ_BT_CLK_IN) */
    uint8_t ck_ch;                       /* SMK-37 clip mode: the keybed MIDI channel 2-15 (0: an older object) */
    uint8_t ck_root, ck_scale, ck_snap;  /* clip mode SCALE: root 0-11, scale, 0 NEAR / 1 FILTER (smk_clip_keys.h) */
    uint8_t ck_chord;                    /* clip mode CHORD: type */
    uint8_t ck_ord, ck_rate, ck_oct;     /* clip mode ARP: order, rate, octaves - 1 (0-3) */
    uint8_t ck_gate;                     /* clip mode ARP: gate / 10 % - 1 (0-9) */
    uint8_t ck_inv;                      /* clip mode CHORD: inversion 0-3 */
    uint8_t ck_x1;                       /* clip mode ARP: bits 0-2 SWING (50 + 3 x %), bits 3-5 HUMAN (0 = off) */
    uint8_t ck_x2;                       /* clip mode: bits 0-3 ARP RHYTHM (CK_RHY_T), bits 4-6 CHORD STRUM (x 10 ms) */
} settings_t;

/* the song and the motion lanes: one blob, split across OBJ_SONG and OBJ_MOTION */
typedef struct {
    song_t song;
    lane_t lane[NLANE];
} arrange_t;

typedef struct {
    settings_t set;
    sound_t sound;
    pattern_t pat[NPAT];
    arrange_t arr;
} project_t;

extern project_t proj;
extern project_t project_stage;          /* exclusive staging, otherwise the UI undo shadow */

void project_defaults(void);
int project_storage_unsupported(void); /* stored format retained; explicit validated adoption required */
int project_load(void);                  /* 0 = loaded; else defaults are in place */
int project_save(void);                  /* 0 = verified; -7 = memory full; -9 = unsupported data retained (nothing written) */
void project_apply_io(void);             /* the settings into the sequencer and the engine: MIDI, the output */
int project_mem_pct(void);               /* how full the room for patterns, song and motion is, % */

/* Call only between ui_project_begin/end: these never change the live project. */
void project_reset_stage(void);
int project_load_stage(void);
void project_capture_stage(void);
int project_validate_stage(void);         /* 0 good, -1 invalid, -7 storage capacity */
void project_commit_stage(void);          /* adopt with the audio engine held */
#define PROJECT_FILE_HDR 32u
#define PROJECT_FILE_SIZE (PROJECT_FILE_HDR + sizeof(project_t))
void project_file_header(uint8_t *header);
int project_file_validate(const uint8_t *header); /* also -4 for a payload CRC mismatch */

void ui_init(void);
void ui_frame(void);
void ui_input_only(void);
int ui_dirty(void);
#define UI_PROJECT_BUSY (-8)
int ui_project_begin(void);               /* 0 acquired, UI_PROJECT_BUSY otherwise */
void ui_project_end(int apply);           /* 0 abort, 1 commit and reset the runtime */
int ui_project_busy(void);
int ui_project_load(void);
int ui_project_reset(void);
int ui_project_save(void);
int ui_boot_flash_busy(void);            /* transport or notes active: refuse boot-screen flash */
void ui_say(const char *a, const char *b);
#if BOARD_SMK37
/* BLE TIMER5 producer; smc_poll and smc_frame consume from the main loop. */
void smc_ble_midi(const uint8_t *msg, uint8_t len);
void smc_ble_link(int up);
int smc_ble_role_get(void);
int smc_connected(void);
int smc_midi_host(uint32_t pkt);       /* audio ISR: opt-in User preset on channel 16 only */
void smc_mmc_host(uint8_t cmd, uint8_t src); /* USB or peripheral BLE IRQ: complete stock User MMC */
void smc_poll(void);
void smc_frame(void);
#endif
