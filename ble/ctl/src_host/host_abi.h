// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 * The image <-> app frames when the image holds JieLi's BLE host (host_app.c). */
#pragma once
#define HX_MTU_MAX 128u
enum { HX_MIDI = 0x20, HX_STATE = 0x21, HX_CMD = 0x22, HX_NAME = 0x23 };
enum { HX_ST_IDLE = 0, HX_ST_ADV = 1, HX_ST_CONN = 2 };
void hx_to_app(unsigned char type, const unsigned char *p, unsigned n);
void hx_from_app(unsigned char type, const unsigned char *p, unsigned n);
void hx_init(void);
