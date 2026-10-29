/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_UI_THEME_H
#define X0X_UI_THEME_H
/* Night materials: compile-time tokens, not another settings or storage format.
 * Existing GLOBAL accent choices remain independent of the shared panel material.
 * RGB565 and the existing strip renderer are the only rendering requirements. */
#define UI_N_BASE RGB(5,10,13)
#define UI_N_FACE RGB(16,26,31)
#define UI_N_EDGE RGB(38,56,61)
#define UI_N_LEFT RGB(28,44,49)
#define UI_N_SIDE RGB(7,14,17)
#define UI_N_SHADOW RGB(2,4,6)
#define UI_N_SELECT RGB(37,59,45)
#define UI_N_SELECT_EDGE RGB(62,92,64)
#define UI_N_SELECT_SIDE RGB(21,39,27)
#define UI_N_INK RGB(224,235,214)
#define UI_N_WHITE RGB(222,231,223)
#define UI_N_GRAY RGB(156,170,168)
#define UI_N_DIM RGB(128,146,148)
#define UI_N_LINE RGB(40,56,61)
#define UI_N_ACCENT RGB(143,191,121)
#define UI_N_AMBER RGB(196,155,69)
#endif
