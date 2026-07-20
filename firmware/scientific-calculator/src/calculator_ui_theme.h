#ifndef CALCULATOR_UI_THEME_H
#define CALCULATOR_UI_THEME_H

#include "lcd_st7796.h"

/* One restrained, high-contrast palette shared by every calculator module. */
#define UI_COLOR_BACKGROUND RGB565(0, 0, 0)
#define UI_COLOR_SURFACE    RGB565(64, 64, 64)
#define UI_COLOR_KEY        RGB565(255, 255, 255)
#define UI_COLOR_FUNCTION   RGB565(205, 205, 205)
#define UI_COLOR_PRIMARY    RGB565(255, 220, 64)
#define UI_COLOR_ACCENT     RGB565(255, 220, 64)
#define UI_COLOR_DANGER     RGB565(240, 92, 92)
#define UI_COLOR_TEXT       RGB565(255, 255, 255)
#define UI_COLOR_TEXT_DARK  RGB565(12, 15, 18)
#define UI_COLOR_MUTED      RGB565(190, 190, 190)
#define UI_COLOR_GRID       RGB565(72, 72, 72)

#define UI_MARGIN 4
#define UI_GAP 4
#define UI_STATUS_HEIGHT 16

#endif
