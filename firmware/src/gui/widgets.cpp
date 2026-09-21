#include "widgets.h"
#include "assets_icons.h"
#include "assets_fonts.h"
#include "../board.h"
#include "../version.h"
#include <Arduino.h>
#include <stdio.h>

static char timebuf[8];

static void clock_text(char* o, size_t n) {
    uint32_t s = millis() / 1000;
    uint32_t m = (s / 60) % 60;
    uint32_t h = (s / 3600) % 24;
    snprintf(o, n, "%02u:%02u", (unsigned)h, (unsigned)m);
}

void statusbar_draw(Canvas& c, const char* title) {
    c.setColor(1);
    if (title && title[0]) {
        c.text(2, 1, title, &tf_primary);
    } else {
        clock_text(timebuf, sizeof(timebuf));
        c.text(2, 1, timebuf, &tf_primary);
    }
    int x = 127;
    /* battery 16x9 */
    x -= 18;
    c.icon(x, 1, &tfi_st_battery);
    int pct = Board::batteryPercent();
    int fill = 1 + (pct * 11) / 100;
    c.fill(x + 1, 2, fill, 5);
    if (Board::usbConnected()) {
        /* charge notch already in icon */
    }
    if (Board::sdPresent()) {
        x -= 10;
        c.icon(x, 1, &tfi_st_sd);
    }
    c.hline(0, 11, 128);
}

void Menu::set(const char* t, const MenuItem* it, int n) {
    title = t; items = it; count = n;
    if (sel >= count) sel = count ? count - 1 : 0;
    if (sel < window) window = sel;
}

void Menu::draw(Canvas& c) {
    c.clear(0);
    statusbar_draw(c, title);
    if (!items || count <= 0) {
        c.text(8, 28, "Empty", &tf_primary);
        return;
    }
    const int row_h = 8;
    const int y0 = 14;
    const int vis = show_icon ? 6 : 5;
    rows = vis;
    if (sel < window) window = sel;
    if (sel >= window + vis) window = sel - vis + 1;
    if (window < 0) window = 0;

    if (show_icon) {
        const TfIcon* ic = items[sel].icon;
        if (ic) {
            /* scale-ish: draw 24x24 at (4, 20) */
            c.icon(4, 22, ic);
        }
        for (int i = 0; i < vis; i++) {
            int idx = window + i;
            if (idx >= count) break;
            int y = y0 + i * row_h;
            if (idx == sel) {
                c.rbox(50, y - 1, 76, row_h, 2);
                c.setColor(0);
                c.text(54, y, items[idx].label, &tf_primary);
                c.setColor(1);
            } else {
                c.text(54, y, items[idx].label, &tf_primary);
            }
        }
        c.scrollbar(126, 13, 50, window, count, vis);
    } else {
        for (int i = 0; i < vis; i++) {
            int idx = window + i;
            if (idx >= count) break;
            int y = y0 + i * 9;
            if (idx == sel) {
                c.rbox(2, y - 1, 124, 9, 2);
                c.setColor(0);
                c.text(6, y, items[idx].label, &tf_primary);
                c.setColor(1);
            } else {
                c.text(6, y, items[idx].label, &tf_primary);
            }
        }
        c.scrollbar(126, 13, 50, window, count, vis);
    }
}

bool Menu::input(const InputEvent& e) {
    if (e.type != InputTypeShort && e.type != InputTypeRepeat && e.type != InputTypePress)
        return false;
    if (e.key == InputKeyUp) {
        if (count) sel = (sel + count - 1) % count;
        gCanvas.markDirty();
        return true;
    }
    if (e.key == InputKeyDown) {
        if (count) sel = (sel + 1) % count;
        gCanvas.markDirty();
        return true;
    }
    return false;
}

void VarList::draw(Canvas& c) {
    c.clear(0);
    statusbar_draw(c, title);
    const int vis = 5;
    int window = 0;
    if (sel >= vis) window = sel - vis + 1;
    for (int i = 0; i < vis; i++) {
        int idx = window + i;
        if (idx >= count) break;
        int y = 14 + i * 10;
        if (idx == sel) {
            c.rbox(1, y - 1, 126, 10, 2);
            c.setColor(0);
        }
        c.text(4, y, items[idx].label, &tf_primary);
        c.textRight(124, y, items[idx].value, &tf_secondary);
        c.setColor(1);
    }
}

bool VarList::input(const InputEvent& e) {
    if (e.type != InputTypeShort && e.type != InputTypeRepeat) return false;
    if (e.key == InputKeyUp) { if (count) sel = (sel + count - 1) % count; gCanvas.markDirty(); return true; }
    if (e.key == InputKeyDown) { if (count) sel = (sel + 1) % count; gCanvas.markDirty(); return true; }
    if ((e.key == InputKeyOk || e.key == InputKeyLeft || e.key == InputKeyRight) && items && items[sel].change) {
        int dir = (e.key == InputKeyLeft) ? -1 : 1;
        items[sel].change(dir, &items[sel]);
        gCanvas.markDirty();
        return true;
    }
    return false;
}
