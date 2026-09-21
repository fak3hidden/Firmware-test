#include "widgets.h"
#include "elements.h"
#include "assets_fonts.h"
#include "assets_menu.h"
#include "../board.h"
#include <stdio.h>

void Menu::set(const char* t, const MenuItem* it, int n) {
    title = t; items = it; count = n;
    if (sel >= count) sel = count ? count - 1 : 0;
}

void Menu::draw(Canvas& c) {
    if (show_icon) {
        menu_draw_ofw(c, items, count, sel);
        return;
    }
    const char* labels[24];
    int n = count < 24 ? count : 24;
    for (int i = 0; i < n; i++) labels[i] = items[i].label;
    submenu_draw_ofw(c, title, labels, n, sel);
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
    /* Flipper variable_item_list: 16px rows, value on the right, 4 on screen. */
    c.clear(0);
    c.setColor(1);
    const int item_h = 16;
    const int on_screen = 4;
    int win = 0;
    if (sel >= on_screen) win = sel - on_screen + 1;
    for (int i = 0; i < on_screen; i++) {
        int idx = win + i;
        if (idx >= count) break;
        int y = i * item_h;
        if (idx == sel) {
            elements_slightly_rounded_box(c, 0, y + 1, 128 - 5, item_h - 2);
            c.setColor(0);
        }
        c.str(6, y + item_h - 4, items[idx].label, &tf_secondary);
        c.str(122 - c.textWidth(items[idx].value, &tf_secondary), y + item_h - 4,
              items[idx].value, &tf_secondary);
        c.setColor(1);
    }
    elements_scrollbar(c, sel, count);
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
