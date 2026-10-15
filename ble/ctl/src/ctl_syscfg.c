// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Settings store calls from the libraries. Stage 2 keeps no store: every read misses and every
 * write fails, so the controller does its full RF trim at each power-on (RFIinitUseTrimValue 0).
 * The RF calibration comes from the boot parameters, which the app fills. */
#include "ctl_port.h"

#ifndef CTL_HOST
int syscfg_read(u16 id, void *buf, u16 len) { (void)id; (void)buf; (void)len; return -1; }
int syscfg_write(u16 id, void *buf, u16 len) { (void)id; (void)buf; (void)len; return -1; }
#else
/* ABI 3: the host's bonds (the SDK's VM items VM_BLE_LOCAL_INFO 109 and VM_BLE_REMOTE_DB_INFO / _00.._09
 * 113..123, include_lib/utils/syscfg/syscfg_id.h) live in the bond store: XB_NV_SIZE bytes at the top of
 * RAM 2 that the app fills from its flash before init and saves when hx.nv_writes changes. Every other
 * item still misses (the RF trim, as in stage 2). Layout: magic, used bytes, then [id, len, data]. */
#define NV_MAGIC 0x564E4258u              /* "XBNV" */
extern u8 __ctl_nv[];
typedef struct { u32 magic; u16 used, rsv; u8 d[XB_NV_SIZE - 8]; } nv_t;
#define NV ((nv_t *)(void *)__ctl_nv)
static int nv_id_ok(u16 id) { return id == 109u || (id >= 113u && id <= 123u); }
static void nv_check(void)
{
    if (NV->magic != NV_MAGIC || NV->used > sizeof NV->d) {
        memset(NV, 0, XB_NV_SIZE);
        NV->magic = NV_MAGIC;
    }
}
static int nv_find(u16 id)                /* offset of the entry, -1 none */
{
    u32 o = 0;
    while (o + 2u <= NV->used) {
        if (NV->d[o] == id) return (int)o;
        o += 2u + NV->d[o + 1];
    }
    return -1;
}
int syscfg_read(u16 id, void *buf, u16 len)
{
    int o;
    u32 n;
    if (!nv_id_ok(id) || !buf) return -1;
    nv_check();
    o = nv_find(id);
    if (o < 0) return -1;
    n = NV->d[o + 1];
    if (n > len) n = len;
    memcpy(buf, &NV->d[o + 2], n);
    return (int)n;
}
int syscfg_write(u16 id, void *buf, u16 len)
{
    int o;
    u32 f;
    if (!nv_id_ok(id) || !buf || len > 255u) return -1;
    nv_check();
    f = ctl_port_irq_save();
    o = nv_find(id);
    if (o >= 0 && NV->d[o + 1] == len && memcmp(&NV->d[o + 2], buf, len) == 0) {
        ctl_port_irq_restore(f);
        return len;                       /* the same: no store change */
    }
    if (o >= 0) {                         /* drop the old entry */
        u32 e = (u32)o + 2u + NV->d[o + 1];
        memmove(&NV->d[o], &NV->d[e], NV->used - e);
        NV->used = (u16)(NV->used - (e - (u32)o));
    }
    if (NV->used + 2u + len > sizeof NV->d) {
        ctl_mb->hx.nv_writes |= 0x80000000u;   /* full: the bond is not kept (DIAG) */
        ctl_port_irq_restore(f);
        return -1;
    }
    NV->d[NV->used] = (u8)id;
    NV->d[NV->used + 1] = (u8)len;
    memcpy(&NV->d[NV->used + 2], buf, len);
    NV->used = (u16)(NV->used + 2u + len);
    ctl_mb->hx.nv_writes++;
    ctl_port_irq_restore(f);
    return len;
}
void ctl_nv_forget(void)
{
    u32 f = ctl_port_irq_save();
    memset(NV, 0, XB_NV_SIZE);
    NV->magic = NV_MAGIC;
    ctl_mb->hx.nv_writes++;
    ctl_port_irq_restore(f);
}
#endif

void wifi_get_xosc(u8 *xosc) { memcpy(xosc, ctl_bp->xosc, 2); }
void wifi_get_mcs_dgain(u8 *g) { memcpy(g, ctl_bp->mcs_dgain, sizeof(ctl_bp->mcs_dgain)); }
int wifi_get_pa_trim_data(u8 *pa)
{
    memcpy(pa, ctl_bp->pa_trim, sizeof(ctl_bp->pa_trim));
    return 1;   /* as the SDK default: no automatic tune */
}
int wifi_get_rf_trim_data(void *info, int size) { (void)info; (void)size; return -1; }
int wifi_set_rf_trim_data(void *info, int size) { (void)info; (void)size; return -1; }

/* RF switches read by wl_rf_common (values chosen for this image). */
const u8 RTDebugLevel = 0;
const char WIFI_PA_ENABLE = 0;
const char wifi_temperature_drift_trim_on = 0;  /* off: no ADC in this image */
const u8 RFIinitUseTrimValue = 0;               /* full RF init at each power-on */
