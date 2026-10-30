/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* Product Miami / WBL1 engine. Device packages (Elite/Pro/DEV) default ON.
 * Host tests default OFF unless -DX0X_WOBBLE=1 (keeps Chicago-only harnesses).
 * Legacy -DX0X_DEV_WOBBLE=1 still enables the product path. */
#if defined(X0X_DEV_WOBBLE) && !defined(X0X_WOBBLE)
# define X0X_WOBBLE X0X_DEV_WOBBLE
#endif
#ifndef X0X_WOBBLE
# ifdef X0X_HOST
#  define X0X_WOBBLE 0
# else
#  define X0X_WOBBLE 1
# endif
#endif
#ifndef X0X_DEV_WOBBLE
# define X0X_DEV_WOBBLE X0X_WOBBLE
#endif
#define WOBBLE_EXPERIMENT_NPARAMS 23u /* 16-byte AU bank + bend/damper + owner ADSR + Bass Split */
