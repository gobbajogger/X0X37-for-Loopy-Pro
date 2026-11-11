/* SPDX-License-Identifier: GPL-3.0-only */
/* Capture the real host framebuffer on base and candidate; not a test bypass. */
#define F4_CAPTURE_DRIVER 1
#include "smc_remainder_test.c"
int main(void) { capture_scenes(); return 0; }
