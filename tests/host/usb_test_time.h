/* SPDX-License-Identifier: GPL-3.0-only */
/* Non-timing USB tests never advance TIMER4. Retry timing is exercised by
 * usb_retry_cooperative_test.c with its own controlled hardware clock. */
#pragma once
#include <stdint.h>
#define FM1_TICKS_PER_US 24u
static uint32_t fm1_ticks(void) { return 0; }
