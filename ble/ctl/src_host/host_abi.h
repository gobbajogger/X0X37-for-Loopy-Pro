// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 * The image side of the ABI 3 frames (firmware/hal/x0x_ble_abi.h): host_app.c <-> ctl_main.c. */
#pragma once
#define HX_MTU_MAX 64u                    /* the ATT MTU the image offers (a packet: up to 61 bytes, at most 30
                                          * messages: the app's 32-message MIDI in queue takes one whole packet) */
void hx_to_app(unsigned char type, const unsigned char *p, unsigned n);   /* ctl_main.c: into c2h */
void hx_from_app(unsigned char type, const unsigned char *p, unsigned n); /* host_app.c: from h2c */
void hx_init(void);
int hx_can_send(unsigned n);              /* host_app.c: room for a notification of n bytes (or no link) */                       /* host_app.c: before btstack_init */
void hx_start(void);                      /* host_app.c: ctl_poll, until the link layer use starts (attempt 50) */
