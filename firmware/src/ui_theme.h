/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef X0X_UI_THEME_H
#define X0X_UI_THEME_H
/* Complete materials follow the existing revision-13 palette byte. */
static uint8_t ui_theme_light;
static const uint16_t ui_material[2][16] = {
    {RGB(5,10,13),RGB(16,26,31),RGB(38,56,61),RGB(28,44,49),RGB(7,14,17),RGB(2,4,6),RGB(37,59,45),RGB(62,92,64),RGB(21,39,27),RGB(224,235,214),RGB(222,231,223),RGB(156,170,168),RGB(128,146,148),RGB(40,56,61),RGB(143,191,121),RGB(196,155,69)},
    {RGB(236,239,232),RGB(250,252,246),RGB(180,193,180),RGB(210,220,206),RGB(166,181,163),RGB(146,161,144),RGB(205,223,196),RGB(88,121,74),RGB(147,176,133),RGB(24,41,31),RGB(24,41,31),RGB(64,81,62),RGB(80,97,77),RGB(180,193,180),RGB(36,91,43),RGB(130,84,14)}
};
#define UI_N_BASE ui_material[ui_theme_light][0]
#define UI_N_FACE ui_material[ui_theme_light][1]
#define UI_N_EDGE ui_material[ui_theme_light][2]
#define UI_N_LEFT ui_material[ui_theme_light][3]
#define UI_N_SIDE ui_material[ui_theme_light][4]
#define UI_N_SHADOW ui_material[ui_theme_light][5]
#define UI_N_SELECT ui_material[ui_theme_light][6]
#define UI_N_SELECT_EDGE ui_material[ui_theme_light][7]
#define UI_N_SELECT_SIDE ui_material[ui_theme_light][8]
#define UI_N_INK ui_material[ui_theme_light][9]
#define UI_N_WHITE ui_material[ui_theme_light][10]
#define UI_N_GRAY ui_material[ui_theme_light][11]
#define UI_N_DIM ui_material[ui_theme_light][12]
#define UI_N_LINE ui_material[ui_theme_light][13]
#define UI_N_ACCENT ui_material[ui_theme_light][14]
#define UI_N_AMBER ui_material[ui_theme_light][15]
#endif
