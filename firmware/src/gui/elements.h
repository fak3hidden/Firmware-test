#pragma once
#include "../canvas.h"
#include "assets_fonts.h"
#include "assets_icons.h"
#include "assets_menu.h"

/* Flipper GUI elements — same geometry as OFW applications/services/gui/elements.c */

void elements_scrollbar(Canvas& c, int pos, int total);
void elements_frame(Canvas& c, int x, int y, int w, int h);
void elements_slightly_rounded_box(Canvas& c, int x, int y, int w, int h);
void elements_slightly_rounded_frame(Canvas& c, int x, int y, int w, int h);
void elements_bold_rounded_frame(Canvas& c, int x, int y, int w, int h);
void elements_button_left(Canvas& c, const char* str);
void elements_button_right(Canvas& c, const char* str);
void elements_button_center(Canvas& c, const char* str);
void elements_progress_bar(Canvas& c, int x, int y, int w, float progress);

/* Status bar is 13px, only used on window-layer views (desktop). */
void statusbar_draw(Canvas& c);
void app_header(Canvas& c, const char* title);

/* Official 3-line main menu (fullscreen, no status bar). */
void menu_draw_ofw(Canvas& c, const struct MenuItem* items, int count, int sel);
/* Official submenu: 16px rows, 4 on screen, inverted selection. */
void submenu_draw_ofw(Canvas& c, const char* header, const char* const* labels, int count, int sel);
