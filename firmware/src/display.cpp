#include "display.h"
#include "board.h"

/* ST7789 170x320, landscape 320x170. Viewport is 256x128 (2x 128x64) centred. */

static constexpr int PW = 320;
static constexpr int PH = 170;
static constexpr int SCALE = 2;
static constexpr int VW = CANVAS_W * SCALE;
static constexpr int VH = CANVAS_H * SCALE;
static constexpr int OX = (PW - VW) / 2; /* 32 */
static constexpr int OY = (PH - VH) / 2; /* 21 */

static constexpr uint16_t COL_ORANGE = 0xFC00; /* Flipper orange-ish */
static constexpr uint16_t COL_BLACK  = 0x0000;
static constexpr uint16_t COL_WHITE  = 0xFFFF;

static uint8_t gRot = 3;
static uint8_t gColStart = 0, gRowStart = 35;

static void dc(int v) { digitalWrite(DISPLAY_DC, v); }

static void cmd(uint8_t c) {
    dc(0);
    Board::spi().transfer(c);
}
static void dat(uint8_t d) {
    dc(1);
    Board::spi().transfer(d);
}
static void dat16(uint16_t d) {
    dc(1);
    Board::spi().transfer((uint8_t)(d >> 8));
    Board::spi().transfer((uint8_t)d);
}

static void window(int x, int y, int w, int h) {
    uint16_t x0 = (uint16_t)(x + gColStart);
    uint16_t x1 = (uint16_t)(x + w - 1 + gColStart);
    uint16_t y0 = (uint16_t)(y + gRowStart);
    uint16_t y1 = (uint16_t)(y + h - 1 + gRowStart);
    cmd(0x2A); dat((uint8_t)(x0 >> 8)); dat((uint8_t)x0); dat((uint8_t)(x1 >> 8)); dat((uint8_t)x1);
    cmd(0x2B); dat((uint8_t)(y0 >> 8)); dat((uint8_t)y0); dat((uint8_t)(y1 >> 8)); dat((uint8_t)y1);
    cmd(0x2C);
}

static void applyRotation(uint8_t rot) {
    /* ST7789 MADCTL. Colour order RGB, inversion on. */
    uint8_t mad;
    switch (rot & 3) {
        case 0: mad = 0x00; gColStart = 35; gRowStart = 0; break; /* 170x320 */
        case 1: mad = 0x60; gColStart = 0;  gRowStart = 35; break; /* 320x170 */
        case 2: mad = 0xC0; gColStart = 35; gRowStart = 0; break;
        default:mad = 0xA0; gColStart = 0;  gRowStart = 35; break; /* rot 3 */
    }
    Board::spiLock();
    Board::selectDisplay();
    cmd(0x36); dat(mad);
    Board::deselectAll();
    Board::spiUnlock();
}

void Display::init() {
    pinMode(DISPLAY_DC, OUTPUT);
    pinMode(DISPLAY_CS, OUTPUT);
    digitalWrite(DISPLAY_CS, HIGH);
    digitalWrite(DISPLAY_DC, HIGH);

    Board::spiLock();
    Board::selectDisplay();
    cmd(0x01); Board::deselectAll(); Board::spiUnlock();
    delay(150);

    Board::spiLock(); Board::selectDisplay();
    cmd(0x11); Board::deselectAll(); Board::spiUnlock();
    delay(120);

    Board::spiLock(); Board::selectDisplay();
    cmd(0x3A); dat(0x55);           /* 16 bpp */
    cmd(0x21);                      /* inversion on (LilyGO) */
    cmd(0x13);
    cmd(0x29);
    Board::deselectAll(); Board::spiUnlock();
    delay(20);

    setRotation(3);
    fillBezel();
}

void Display::setRotation(uint8_t rot) {
    gRot = rot & 3;
    applyRotation(gRot);
}
uint8_t Display::rotation() { return gRot; }
int Display::panelW() { return (gRot & 1) ? PW : PH; }
int Display::panelH() { return (gRot & 1) ? PH : PW; }

void Display::fillBezel() {
    Board::spiLock();
    Board::selectDisplay();
    int w = panelW(), h = panelH();
    window(0, 0, w, h);
    dc(1);
    SPIClass& spi = Board::spi();
    uint32_t n = (uint32_t)w * (uint32_t)h;
    uint8_t pair[2] = { (uint8_t)(COL_ORANGE >> 8), (uint8_t)COL_ORANGE };
    /* write in chunks to keep stack small */
    static uint8_t chunk[512];
    for (int i = 0; i < 256; i++) { chunk[i * 2] = pair[0]; chunk[i * 2 + 1] = pair[1]; }
    uint32_t left = n;
    while (left) {
        uint32_t k = left > 256 ? 256 : left;
        spi.writeBytes(chunk, k * 2);
        left -= k;
    }
    Board::deselectAll();
    Board::spiUnlock();
}

void Display::present(const Canvas& c) {
    if (!c.dirty) return;
    presentForce(c);
    const_cast<Canvas&>(c).dirty = false;
}

void Display::presentForce(const Canvas& c) {
    /* 2x nearest-neighbour blit of 128x64 into 256x128 window. */
    Board::spiLock();
    Board::selectDisplay();
    window(OX, OY, VW, VH);
    dc(1);
    SPIClass& spi = Board::spi();
    static uint8_t line[VW * 2]; /* one RGB565 row = 512 bytes */
    for (int y = 0; y < CANVAS_H; y++) {
        int n = 0;
        for (int x = 0; x < CANVAS_W; x++) {
            uint8_t ink = (c.buf[y * 16 + (x >> 3)] >> (7 - (x & 7))) & 1;
            uint16_t col = ink ? COL_BLACK : COL_WHITE;
            uint8_t hi = (uint8_t)(col >> 8), lo = (uint8_t)col;
            line[n++] = hi; line[n++] = lo;
            line[n++] = hi; line[n++] = lo;
        }
        /* two identical dest rows */
        for (int rep = 0; rep < SCALE; rep++) {
            spi.writeBytes(line, VW * 2);
        }
    }
    Board::deselectAll();
    Board::spiUnlock();
}
