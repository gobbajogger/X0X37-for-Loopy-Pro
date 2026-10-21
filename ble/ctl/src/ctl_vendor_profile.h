// SPDX-License-Identifier: MPL-2.0
#ifndef CTL_VENDOR_PROFILE_H
#define CTL_VENDOR_PROFILE_H
#include <stdint.h>
/* Core1 only, after power/trim/IRQ setup. Owns the sole controller startup. */
int ctl_vendor_startup(void);
void ctl_vendor_poll(void);
int ctl_vendor_command(uint8_t type, const uint8_t *payload, uint32_t length);
int ctl_vendor_scan(int enable);
int ctl_vendor_connect(uint8_t address_type, const uint8_t address[6]);
int ctl_vendor_disconnect(void);
int ctl_vendor_write(const uint8_t *payload, uint16_t length);
uint16_t ctl_vendor_connection(void); /* 0xffff means absent; handle0 is valid */
uint16_t ctl_vendor_midi_handle(void);
/* Required bridge: copy before returning; no pointer may cross cores. */
int ctl_vendor_notification_copy(const uint8_t *payload, uint32_t length);
uint32_t ctl_vendor_session_next(void);
int ctl_vendor_link_copy(uint32_t epoch, uint8_t up, uint8_t reason);
#endif
