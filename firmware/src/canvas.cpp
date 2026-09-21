#include "canvas.h"
#include <string.h>
#include <stdlib.h>

Canvas gCanvas;

void Canvas::clear(uint8_t ink) {
    memset(buf, ink ? 0xFF : 0x00, CANVAS_BUF);
    dirty = true;
}

static inline bool inb(int x, int y) {
    return (unsigned)x < CANVAS_W && (unsigned)y < CANVAS_H;
}

void Canvas::pixel(int x, int y) { pixel(x, y, color); }

void Canvas::pixel(int x, int y, uint8_t c) {
    if (!inb(x, y)) return;
    /* row-major, MSB first: byte = y * 16 + x/8, bit = 7 - x%8 */
    uint8_t* p = &buf[y * (CANVAS_W / 8) + (x >> 3)];
    uint8_t m = (uint8_t)(0x80 >> (x & 7));
    if (c) *p |= m;
    else   *p &= (uint8_t)~m;
}

uint8_t Canvas::get(int x, int y) const {
    if (!inb(x, y)) return 0;
    uint8_t b = buf[y * (CANVAS_W / 8) + (x >> 3)];
    return (b >> (7 - (x & 7))) & 1;
}

void Canvas::hline(int x, int y, int w) {
    for (int i = 0; i < w; i++) pixel(x + i, y);
}
void Canvas::vline(int x, int y, int h) {
    for (int i = 0; i < h; i++) pixel(x, y + i);
}

void Canvas::line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        pixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Canvas::rect(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    hline(x, y, w);
    hline(x, y + h - 1, w);
    vline(x, y, h);
    vline(x + w - 1, y, h);
}

void Canvas::fill(int x, int y, int w, int h) {
    for (int j = 0; j < h; j++) hline(x, y + j, w);
}

void Canvas::rframe(int x, int y, int w, int h, int r) {
    if (w <= 0 || h <= 0) return;
    if (r < 0) r = 0;
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    hline(x + r, y, w - 2 * r);
    hline(x + r, y + h - 1, w - 2 * r);
    vline(x, y + r, h - 2 * r);
    vline(x + w - 1, y + r, h - 2 * r);
    auto oct = [&](int cx, int cy, int ox, int oy) {
        pixel(cx + ox, cy - oy);
        pixel(cx + oy, cy - ox);
        pixel(cx - ox, cy - oy);
        pixel(cx - oy, cy - ox);
        pixel(cx + ox, cy + oy);
        pixel(cx + oy, cy + ox);
        pixel(cx - ox, cy + oy);
        pixel(cx - oy, cy + ox);
    };
    int xx = r, yy = 0, err = 1 - r;
    while (xx >= yy) {
        /* only the four corners, using shifted centres */
        pixel(x + r + yy, y + r - xx);
        pixel(x + r + xx, y + r - yy);
        pixel(x + w - 1 - r - yy, y + r - xx);
        pixel(x + w - 1 - r - xx, y + r - yy);
        pixel(x + r + yy, y + h - 1 - r + xx);
        pixel(x + r + xx, y + h - 1 - r + yy);
        pixel(x + w - 1 - r - yy, y + h - 1 - r + xx);
        pixel(x + w - 1 - r - xx, y + h - 1 - r + yy);
        yy++;
        if (err < 0) err += 2 * yy + 1;
        else { xx--; err += 2 * (yy - xx) + 1; }
    }
    (void)oct;
}

void Canvas::rbox(int x, int y, int w, int h, int r) {
    if (w <= 0 || h <= 0) return;
    if (r < 0) r = 0;
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    fill(x + r, y, w - 2 * r, h);
    fill(x, y + r, r, h - 2 * r);
    fill(x + w - r, y + r, r, h - 2 * r);
    for (int oy = 0; oy < r; oy++) {
        for (int ox = 0; ox < r; ox++) {
            int dx = r - 1 - ox, dy = r - 1 - oy;
            if (dx * dx + dy * dy <= r * r) {
                pixel(x + ox, y + oy);
                pixel(x + w - 1 - ox, y + oy);
                pixel(x + ox, y + h - 1 - oy);
                pixel(x + w - 1 - ox, y + h - 1 - oy);
            }
        }
    }
}

void Canvas::invert(int x, int y, int w, int h) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int xx = x + i, yy = y + j;
            if (!inb(xx, yy)) continue;
            uint8_t* p = &buf[yy * (CANVAS_W / 8) + (xx >> 3)];
            *p ^= (uint8_t)(0x80 >> (xx & 7));
        }
    }
}

void Canvas::icon(int x, int y, const TfIcon* ic, bool inv) {
    if (!ic || !ic->data) return;
    int stride = (ic->w + 7) / 8;
    for (int j = 0; j < ic->h; j++) {
        for (int i = 0; i < ic->w; i++) {
            uint8_t bit = (ic->data[j * stride + (i >> 3)] >> (7 - (i & 7))) & 1;
            if (inv) bit = (uint8_t)!bit;
            if (bit) pixel(x + i, y + j);
        }
    }
}

int Canvas::textWidth(const char* s, const TfFont* f) const {
    if (!s || !f) return 0;
    int w = 0;
    for (; *s; s++) {
        unsigned c = (unsigned char)*s;
        if (c < f->first_char || c > f->last_char) { w += 4; continue; }
        w += f->adv[c - f->first_char];
    }
    return w;
}

int Canvas::text(int x, int y, const char* s, const TfFont* f) {
    if (!s || !f) return 0;
    int px = x;
    int baseline = y + f->cap;
    for (; *s; s++) {
        unsigned c = (unsigned char)*s;
        if (c < f->first_char || c > f->last_char) { px += 4; continue; }
        int i = c - f->first_char;
        int gw = f->widths[i], gh = f->heights[i];
        if (gw == 0) { px += f->adv[i]; continue; }
        const uint8_t* data = f->data + f->offsets[i];
        int stride = (gw + 7) / 8;
        int ix = px + f->dx[i];
        int iy = baseline + f->dy[i];
        for (int j = 0; j < gh; j++) {
            for (int k = 0; k < gw; k++) {
                if ((data[j * stride + (k >> 3)] >> (7 - (k & 7))) & 1)
                    pixel(ix + k, iy + j);
            }
        }
        px += f->adv[i];
    }
    return px - x;
}

int Canvas::textRight(int x, int y, const char* s, const TfFont* f) {
    int w = textWidth(s, f);
    return text(x - w, y, s, f);
}
int Canvas::textCenter(int x, int y, const char* s, const TfFont* f) {
    int w = textWidth(s, f);
    return text(x - w / 2, y, s, f);
}

void Canvas::scrollbar(int x, int y, int h, int idx, int count, int visible) {
    if (count <= visible || h < 4) return;
    vline(x, y, h);
    int box = h * visible / count;
    if (box < 4) box = 4;
    int maxo = count - visible;
    int pos = (maxo > 0) ? (h - box) * idx / maxo : 0;
    fill(x - 1, y + pos, 3, box);
}

void Canvas::disc(int cx, int cy, int r) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) pixel(cx + x, cy + y);
        }
    }
}
void Canvas::circle(int cx, int cy, int r) {
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        pixel(cx + x, cy + y); pixel(cx + y, cy + x);
        pixel(cx - y, cy + x); pixel(cx - x, cy + y);
        pixel(cx - x, cy - y); pixel(cx - y, cy - x);
        pixel(cx + y, cy - x); pixel(cx + x, cy - y);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}
