/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#ifndef X0X_DEV_WOBBLE
#define X0X_DEV_WOBBLE 0
#endif
#if X0X_DEV_WOBBLE && (!defined(X0X_DEV_PADCAL) || !X0X_DEV_PADCAL || defined(X0X_VERSION))
#error "Experimental wobble requires unversioned DEV with X0X_DEV_PADCAL=1"
#endif
