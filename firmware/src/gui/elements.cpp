#include "elements.h"
#include "widgets.h"
#include "../board.h"
#include "../ble.h"
#include <stdio.h>

void elements_scrollbar(Canvas& c, int pos, int total) {
    c.setColor(0);
    c.fill(CANVAS_W - 3, 0, 3, CANVAS_H);
    c.setColor(1);
    for (int i = 0; i < CANVAS_H; i += 2) c.pixel(CANVAS_W - 2, i);
    if (total > 0) {
        int block = CANVAS_H / total;
        if (block < 1) block = 1;
        int y = (int)((float)CANVAS_H * pos / total);
        if (y + block > CANVAS_H) y = CANVAS_H - block;
        c.fill(CANVAS_W - 3, y, 3, block);
    }
}

void elements_frame(Canvas& c, int x, int y, int w, int h) {
    /* Distinctive Flipper "shadow" frame (elements_frame). */
    c.setColor(1);
    c.hline(x + 2, y, w - 4);
    c.hline(x + 1, y + h - 1, w - 1);
    c.hline(x + 2, y + h, w - 3);
    c.vline(x, y + 2, h - 4);
    c.vline(x + w - 1, y + 1, h - 3);
    c.vline(x + w, y + 2, h - 4);
    c.pixel(x + 1, y + 1);
}

void elements_slightly_rounded_box(Canvas& c, int x, int y, int w, int h) {
    c.rbox(x, y, w, h, 1);
}
void elements_slightly_rounded_frame(Canvas& c, int x, int y, int w, int h) {
    c.rframe(x, y, w, h, 1);
}

void elements_bold_rounded_frame(Canvas& c, int x, int y, int w, int h) {
    c.setColor(0);
    c.fill(x + 2, y + 2, w - 3, h - 3);
    c.setColor(1);
    c.hline(x + 3, y, w - 6);
    c.hline(x + 2, y + 1, w - 4);
    c.vline(x, y + 3, h - 6);
    c.vline(x + 1, y + 2, h - 4);
    c.vline(x + w, y + 3, h - 6);
    c.vline(x + w - 1, y + 2, h - 4);
    c.hline(x + 3, y + h, w - 6);
    c.hline(x + 2, y + h - 1, w - 4);
}

void elements_button_left(Canvas& c, const char* str) {
    const int bh = 12, vo = 3, ho = 3;
    int sw = c.textWidth(str, &tf_secondary);
    int iw = 4;
    int bw = sw + ho * 2 + iw + 3;
    int y = CANVAS_H;
    c.setColor(1);
    c.fill(0, y - bh, bw, bh);
    c.vline(bw, y - bh, bh);
    c.vline(bw + 1, y - bh + 1, bh - 1);
    c.vline(bw + 2, y - bh + 2, bh - 2);
    c.setColor(0);
    c.icon(ho, y - vo - 7, &tfi_btn_left);
    c.str(ho + iw + 3, y - vo, str, &tf_secondary);
    c.setColor(1);
}

void elements_button_right(Canvas& c, const char* str) {
    const int bh = 12, vo = 3, ho = 3;
    int sw = c.textWidth(str, &tf_secondary);
    int iw = 4;
    int bw = sw + ho * 2 + iw + 3;
    int x = CANVAS_W, y = CANVAS_H;
    c.setColor(1);
    c.fill(x - bw, y - bh, bw, bh);
    c.vline(x - bw - 1, y - bh, bh);
    c.vline(x - bw - 2, y - bh + 1, bh - 1);
    c.vline(x - bw - 3, y - bh + 2, bh - 2);
    c.setColor(0);
    c.str(x - bw + ho, y - vo, str, &tf_secondary);
    c.icon(x - ho - iw, y - vo - 7, &tfi_btn_right);
    c.setColor(1);
}

void elements_button_center(Canvas& c, const char* str) {
    const int bh = 12, vo = 3, ho = 1;
    int sw = c.textWidth(str, &tf_secondary);
    int iw = 7;
    int bw = sw + ho * 2 + iw + 3;
    int x = (CANVAS_W - bw) / 2, y = CANVAS_H;
    c.setColor(1);
    c.fill(x, y - bh, bw, bh);
    c.vline(x - 1, y - bh, bh);
    c.vline(x - 2, y - bh + 1, bh - 1);
    c.vline(x - 3, y - bh + 2, bh - 2);
    c.vline(x + bw, y - bh, bh);
    c.vline(x + bw + 1, y - bh + 1, bh - 1);
    c.vline(x + bw + 2, y - bh + 2, bh - 2);
    c.setColor(0);
    c.icon(x + ho, y - vo - 7, &tfi_btn_center);
    c.str(x + ho + iw + 3, y - vo, str, &tf_secondary);
    c.setColor(1);
}

void elements_progress_bar(Canvas& c, int x, int y, int w, float progress) {
    int h = 9;
    if (progress < 0) progress = 0;
    if (progress > 1) progress = 1;
    c.setColor(0);
    c.fill(x + 1, y + 1, w - 2, h - 2);
    c.setColor(1);
    c.rframe(x, y, w, h, 3);
    int pw = (int)(progress * (w - 2) + 0.5f);
    c.fill(x + 1, y + 1, pw, h - 2);
}

void statusbar_draw(Canvas& c) {
    /* 13px Flipper status bar: bubble background, time left, icons right. */
    c.setColor(0);
    c.fill(0, 0, 128, 13);
    c.setColor(1);
    /* I_Background_128x11 approximation */
    c.rbox(0, 0, 70, 11, 3);
    c.rbox(72, 0, 56, 11, 3);
    c.setColor(0);
    c.fill(2, 2, 66, 7);
    c.fill(74, 2, 52, 7);
    c.setColor(1);
    char t[8];
    uint32_t s = millis() / 1000;
    snprintf(t, sizeof(t), "%02u:%02u", (unsigned)((s / 3600) % 24), (unsigned)((s / 60) % 60));
    c.str(4, 9, t, &tf_secondary);

    int x = 126;
    /* battery */
    x -= 16;
    c.icon(x, 2, &tfi_st_battery);
    int pct = Board::batteryPercent();
    int fill = 1 + (pct * 10) / 100;
    c.fill(x + 1, 3, fill, 5);
    if (Ble::connected() || Ble::enabled()) {
        x -= 10;
        c.icon(x, 1, &tfi_st_bt);
    }
    if (Board::sdPresent()) {
        x -= 10;
        c.icon(x, 1, &tfi_st_sd);
    }
}

void app_header(Canvas& c, const char* title) {
    c.setColor(1);
    if (title && title[0]) c.str(4, 11, title, &tf_primary);
}

void menu_draw_ofw(Canvas& c, const MenuItem* items, int count, int sel) {
    c.clear(0);
    c.setColor(1);
    if (!items || count <= 0) {
        c.str(2, 32, "Empty", &tf_primary);
        elements_scrollbar(c, 0, 0);
        return;
    }
    auto row = [&](int idx, int icon_y, int text_base, const TfFont* font) {
        int i = (idx + count) % count;
        if (items[i].icon) c.icon(4, icon_y, items[i].icon);
        c.str(22, text_base, items[i].label, font);
    };
    row(sel - 1, 3, 14, &tf_secondary);
    row(sel, 25, 36, &tf_primary);
    row(sel + 1, 47, 58, &tf_secondary);
    elements_frame(c, 0, 21, 128 - 5, 21);
    elements_scrollbar(c, sel, count);
}

void submenu_draw_ofw(Canvas& c, const char* header, const char* const* labels, int count, int sel) {
    c.clear(0);
    c.setColor(1);
    const int item_h = 16;
    bool has_h = header && header[0];
    int on_screen = has_h ? 3 : 4;
    int yoff = has_h ? 16 : 0;
    if (has_h) c.str(4, 11, header, &tf_primary);
    int win = 0;
    if (sel >= on_screen) win = sel - on_screen + 1;
    for (int i = 0; i < on_screen; i++) {
        int idx = win + i;
        if (idx >= count) break;
        int y = yoff + i * item_h;
        if (idx == sel) {
            elements_slightly_rounded_box(c, 0, y + 1, 128 - 5, item_h - 2);
            c.setColor(0);
            c.str(6, y + item_h - 4, labels[idx], &tf_secondary);
            c.setColor(1);
        } else {
            c.str(6, y + item_h - 4, labels[idx], &tf_secondary);
        }
    }
    elements_scrollbar(c, sel, count);
}
