// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * Settings store calls from the libraries. Stage 2 keeps no store: every read misses and every
 * write fails, so the controller does its full RF trim at each power-on (RFIinitUseTrimValue 0).
 * The RF calibration comes from the boot parameters, which the app fills. */
#include "ctl_port.h"

int syscfg_read(u16 id, void *buf, u16 len) { (void)id; (void)buf; (void)len; return -1; }
int syscfg_write(u16 id, void *buf, u16 len) { (void)id; (void)buf; (void)len; return -1; }

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
