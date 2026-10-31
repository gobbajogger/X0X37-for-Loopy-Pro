/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
/* This is the existing firmware version define, shared by the UI and USB descriptor.
 * tools/build.py sets numeric X.Y.Z for release and nightly builds.
 * Display text never selects a device or changes the loader identity. */
#ifndef X0X_VERSION
#define X0X_VERSION "DEV"
#endif
#ifndef X0X_GIT_SHA
#define X0X_GIT_SHA ""
#endif

