/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X firmware for the M-VAVE FM-1: one compilation unit for the platform and the app
 * (Felucca's layout: the HAL is header-only, so everything that touches it is here).
 * The DSP (dsp/), the sequencer (seq/) and the audio engine (app/engine.c) are separate
 * units compiled at -O2 by tools/build.py; this one is -Os. Order matters. */
#include <stdint.h>
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_guard.h"
#if BOARD_SMK37
#define SMK_BIG __attribute__((section(".pool")))   /* SMK buffers in the pool (zeroed at boot): .data+.bss is full */
#include "smk_input.h"               /* SMK-37 Pro: keybed, pads, buttons, encoders, faders, wheels (fm1_input.h API) */
#else
#include "fm1_input.h"
#endif
#include "fm1_timer.h"
#include "fm1_audio.h"
#if !BOARD_SMK37
#include "fm1_adc.h"                 /* (SMK: smk_input.h stands in for it) */
#endif
#include "fm1_lcd_hw.h"
#include "fm1_cpu1.h"
#include "fm1_flash.h"
/* Production admission wiring; standalone host modules default to no maintenance. */
#define ST_MAINTENANCE_ADMIT() fm1_capture_pre_admit()
#define OTA_MAINTENANCE_ADMIT() fm1_capture_pre_admit()
#define ED_MAINTENANCE_ADMIT() fm1_capture_pre_admit()
#define ED_MAINTENANCE_HELD() fm1_capture_maintenance_held()
#include "fm1_xip.h"
#include "fm1_perf.h"

#include "libc.c"
#include "lcd.c"
#include "gfx.c"

static volatile uint32_t fm1_ms;  /* milliseconds since boot (TIMER4-based, TIMER5 ISR) */
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static inline int32_t clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

#include "app/plat.h"

#define FELUCCA_OTA 1             /* M-UPGRADE update entry: keeps the web installer working */
#ifndef X0X_CDC
#define X0X_CDC 0                 /* USB serial console off: one plain MIDI interface */
#endif
#define FELUCCA_CDC X0X_CDC
#ifndef X0X_UAC
#define X0X_UAC 1                 /* USB audio input: the master output, recordable over the cable */
#endif
#define FELUCCA_UAC X0X_UAC
#ifndef FELUCCA_ID
#if BOARD_SMK37
#define FELUCCA_ID "SMK-37 Pro_900"   /* package identity (tools/build.py) */
#else
#define FELUCCA_ID "FM-1_900"     /* package identity (tools/build.py) */
#endif
#endif
#include "usb.c"
#ifndef X0X_TRS
#if BOARD_SMK37
#define X0X_TRS 0                 /* SMK-37 Pro: no MIDI IN (its jack is MIDI OUT, UART1 TX on PC7); PH8 is a
                                   * key-matrix row there and must never become UART RX */
#else
#define X0X_TRS 1                 /* MIDI IN on the TRS jack */
#endif
#endif
#if BOARD_SMK37 && X0X_TRS
#error "BOARD_SMK37: midi_uart.c is the FM-1 MIDI IN driver (RX on PH8, a key-matrix row on the SMK)"
#endif
#if X0X_TRS
#include "midi_uart.c"
#endif

static uint8_t flash_ok;          /* JEDEC id matched at boot */
static void audio_silence(void);
static int st_read(uint32_t off, void *dst, uint32_t n)   /* 256-byte IRQ-off windows: audio keeps up */
{
    uint8_t *d = dst;
    int rc = 0;
    (void)FL_PARK();                         /* a Bluetooth controller on core 1: in RAM meanwhile */
    while (n) {
        uint32_t k = n > 256u ? 256u : n, f = irq_save();
        rc = FL_FAR(fl_read_ram)(off, d, k);
        irq_restore(f);
        if (rc)
            break;
        off += k;
        d += k;
        n -= k;
    }
    FL_UNPARK();
    return rc;
}
static int st_erase(uint32_t off)
{
    uint32_t took;
    if (!FL_STORE_OK(off, 0x1000u))
        return -8;
    audio_silence();
    return fl_erase4k(off, &took);
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    if (!FL_STORE_OK(off, n))
        return -8;
    return fl_write(off, src, n);
}
#define ST_IDLE() fm1_wdt_feed()   /* (storage.c, SMK store owner check: between sector erases) */
#include "storage.c"

struct bootguard_s { uint32_t magic, failed, pending; };
extern struct bootguard_s bootguard;
/* One 4 KiB scratch for main-loop jobs that never use the same bytes at the same time:
 *   0x000-0xFFF  the LZ hash table of a project save (lz.c);
 *   0x000-0x7FF  the read buffers of an update session (ota.c);
 *   0x000-0xCEF  the PERF TEST patterns and sound (ui.c, PERF_SCRATCH);
 *   0xD00-0xF7F  the decoded host message of ota.c (OTA_FRAME_MAX), read only in the call that decoded it;
 *   0xD00-0xEFF  editor sample/project chunk staging (editor.c, ED_SMP_SCRATCH), 512 B — same
 *                ownership class as ota_dec: live only inside one ed_handle. ed_service runs
 *                before ota_service; an editor frame is consumed before ota_take can write
 *                ota_dec. SAVE/LOAD paths that call lz_compress do not use ed_smp_buf. An
 *                update session does not call ed_service (blocked in ota_session / ota_idle).
 *   0xF80-0xFEF  the update parameter block (ota.c, 112 B). It is filled only after ota_stage
 *                returns, and ota_commit resets before the main loop can save. A save's LZ table
 *                uses this tail at every other time. ota_dec stops at 0xF7F, so a frame cannot
 *                reach it. The audio interrupt does not use this scratch.
 * A save does not run while the PERF TEST plays its patterns (seq.playing blocks it), and the test
 * makes its patterns after the last possible save (ui.c perf_test_tick, phase 0). An update session
 * stops the sequencer first, and the test then stops and puts the project back. */
_Static_assert(0xD00 + 640 <= 4096, "scratch");
_Static_assert(512u <= 640u, "ed_smp_buf fits the ota_dec region");
static uint16_t x0x_main_scratch[2048] __attribute__((aligned(4)));
#define OTA_SCRATCH ((uint8_t *)x0x_main_scratch)
#define OTA_DEC_OFF 0xD00u
#define LZ_SCRATCH x0x_main_scratch
#define PERF_SCRATCH x0x_main_scratch
#define ED_SMP_SCRATCH (OTA_SCRATCH + OTA_DEC_OFF)   /* 512 B; call-scoped like ota_dec */
#define OTA_PARM_OFF 0xF80u
#define OTA_PARM_N 112u
_Static_assert(OTA_DEC_OFF + 640u <= OTA_PARM_OFF, "update parm overlaps the decoded frame");
_Static_assert(OTA_PARM_OFF + OTA_PARM_N <= sizeof x0x_main_scratch, "update parm fits the scratch");
#include "ota.c"
static uint32_t ota_now_ms(void) { return fm1_ms; }
static void ota_idle(void) { fm1_wdt_feed(); }
static int ota_in_area(uint32_t off, uint32_t n) { return FL_IN(off, n, OTA_AREA, OTA_AREA + OTA_AREA_LEN); }
static int ota_erase(uint32_t off)
{
    uint32_t took;
    if (!ota_in_area(off, 0x1000u) || (off & 0xFFFu))
        return -8;
    audio_silence();
    return fl_erase4k(off, &took);
}
static int ota_prog(uint32_t off, const void *p, uint32_t n)
{
    if (!ota_in_area(off, n))
        return -8;
    return fl_write(off, p, n);
}
static int ota_fread(uint32_t off, void *p, uint32_t n) { return st_read(off, p, n); }
static void ota_show(uint32_t step, int32_t code)
{
#if BOARD_SMK37   /* step 5 stays on screen while the loader writes the flash: say so */
    static const char *const STEP[] = {"", "PACKAGE", "CHECK HEAD", "LOADER", "CONFIRM", "UPDATING..."};
#else
    static const char *const STEP[] = {"", "PACKAGE", "CHECK HEAD", "LOADER", "CONFIRM", "RESTART"};
#endif
    lcd_fill(0, 0, 240, 240, C_BLACK);
    draw_text_box(0, 92, 240, &FONT_S, "UPDATE", C_WHITE, 1);
    if (step < 9u) {
        draw_text_box(0, 124, 240, &FONT_S, STEP[step < 6u ? step : 0], C_HI, 1);
        return;
    }
    draw_text_box(0, 124, 240, &FONT_S, code == 1 ? "DRY RUN OK" : "FAILED", C_HI, 1);
    fm1_delay_ms(1500);
}
static void ota_commit(const uint8_t *parm)
{
    bootguard.pending = 0;                              /* intentional reset */
    bootguard.failed = 0;                               /* a new install starts with a clean slate */
    usb_detach();
    fm1_delay_ms(30);
    fm1_enter_update(parm);
}

#ifndef X0X_BLE
#define X0X_BLE BOARD_SMK37          /* Bluetooth MIDI through a separate controller image (src/ble.c) */
#endif
#if X0X_BLE
#include "ble.c"
#endif

#include "app/slots.c"
#include "app/x0x.h"
#include "app/smk_clip_arp_share.c"   /* one ck_arp_pick + CK_RATE_T for engine + clip MIDI */
#include "app/smc_fx_map_tables.c"    /* one SMC FX map table for engine + smc_mixer */
#include "app/factory_banks.c"       /* built-in Chicago/Miami factory banks + user import bank */
#include "app/panel.c"
#include "app/plat_fm1.c"
/* Project objects are built in the store buffer. st_save reads the old copy's
 * sequence without loading its payload, so those new bytes stay. A boot upload
 * holds the buffer; project_save returns before it writes. Panel save passes
 * its own struct and copies in. The audio interrupt does not use this buffer. */
#define PROJECT_IO_BUFFER st_buf
_Static_assert(sizeof st_buf >= PLAT_STORE_MAX, "project object fits the store buffer");
#include "app/project.c"
#include "app/ui.c"
#if BOARD_SMK37
#include "app/smc_mixer.c"
#endif
#include "app/editor.c"
#include "app/main_fm1.c"
