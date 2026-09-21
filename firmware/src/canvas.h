#pragma once
#include <stdint.h>
#include <stddef.h>
#include "gui/assets.h"

/* 128x64 1-bit canvas. Bit 1 = ink (black on white, Flipper convention). */
static constexpr int CANVAS_W = 128;
static constexpr int CANVAS_H = 64;
static constexpr int CANVAS_BUF = (CANVAS_W * CANVAS_H) / 8;

class Canvas {
public:
    uint8_t buf[CANVAS_BUF];
    bool dirty = true;
    uint8_t color = 1; /* 1 = set ink, 0 = clear */

    void clear(uint8_t ink = 0);
    void setColor(uint8_t c) { color = c ? 1 : 0; }
    void invertColor() { color = color ? 0 : 1; }
    void pixel(int x, int y);
    void pixel(int x, int y, uint8_t c);
    uint8_t get(int x, int y) const;
    void hline(int x, int y, int w);
    void vline(int x, int y, int h);
    void line(int x0, int y0, int x1, int y1);
    void rect(int x, int y, int w, int h);
    void fill(int x, int y, int w, int h);
    void frame(int x, int y, int w, int h) { rect(x, y, w, h); }
    void rframe(int x, int y, int w, int h, int r);
    void rbox(int x, int y, int w, int h, int r);
    void invert(int x, int y, int w, int h);
    void icon(int x, int y, const TfIcon* ic, bool invert = false);
    int  text(int x, int y, const char* s, const TfFont* f);          /* y = top */
    int  str(int x, int baseline, const char* s, const TfFont* f);    /* Flipper canvas_draw_str */
    int  textRight(int x, int y, const char* s, const TfFont* f);
    int  textCenter(int x, int y, const char* s, const TfFont* f);
    int  textWidth(const char* s, const TfFont* f) const;
    void scrollbar(int x, int y, int h, int idx, int count, int visible);
    void disc(int x, int y, int r);
    void circle(int x, int y, int r);
    void markDirty() { dirty = true; }
};

extern Canvas gCanvas;
