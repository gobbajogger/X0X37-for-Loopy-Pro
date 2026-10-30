/* SPDX-License-Identifier: GPL-3.0-only
 * Code-size helper. FM1_BASE(p): the compiler forgets where the pointer p points, so it keeps p in a
 * register and reaches the fields as p + offset (short code), not a full address for each field. */
#pragma once
#define FM1_BASE(p) __asm__("" : "+r"(p))
