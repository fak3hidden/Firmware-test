#include "../scene.h"
#include "../gui/widgets.h"
#include "../gui/elements.h"
#include "../gui/assets_fonts.h"

static const char *gTitle, *gBody, *gLeft, *gRight;
static void (*gOnL)();
static void (*gOnR)();
static int gFocus = 1;

static void enter() { gFocus = 1; }
static void draw(Canvas& c) {
    c.clear(0);
    c.setColor(1);
    elements_bold_rounded_frame(c, 4, 4, 120, 42);
    c.str(10, 18, gTitle ? gTitle : "", &tf_primary);
    c.str(10, 30, gBody ? gBody : "", &tf_secondary);
    if (gLeft) elements_button_left(c, gLeft);
    if (gRight) elements_button_right(c, gRight);
}
static void input(const InputEvent& e) {
    if (e.type != InputTypeShort && e.type != InputTypePress) return;
    if (e.key == InputKeyLeft || e.key == InputKeyUp) { gFocus = 0; gCanvas.markDirty(); }
    if (e.key == InputKeyRight || e.key == InputKeyDown) { gFocus = 1; gCanvas.markDirty(); }
    if (e.key == InputKeyBack) { Scenes::pop(); if (gOnL) gOnL(); return; }
    if (e.key == InputKeyOk) {
        Scenes::pop();
        if (gFocus == 0) { if (gOnL) gOnL(); }
        else { if (gOnR) gOnR(); }
    }
}
const Scene scene_dialog = { "Dialog", enter, nullptr, draw, input, nullptr };

void dialog_show(const char* title, const char* body,
                 const char* left, const char* right,
                 void (*on_left)(), void (*on_right)()) {
    gTitle = title; gBody = body; gLeft = left; gRight = right;
    gOnL = on_left; gOnR = on_right; gFocus = 1;
    Scenes::push(&scene_dialog);
}

static const char *pTitle, *pBody;
static uint32_t pUntil;

static void penter() {}
static void pdraw(Canvas& c) {
    c.clear(0);
    c.setColor(1);
    elements_bold_rounded_frame(c, 8, 12, 112, 32);
    c.str(64 - c.textWidth(pTitle ? pTitle : "", &tf_primary) / 2, 24,
          pTitle ? pTitle : "", &tf_primary);
    c.str(64 - c.textWidth(pBody ? pBody : "", &tf_secondary) / 2, 36,
          pBody ? pBody : "", &tf_secondary);
}
static void pinput(const InputEvent& e) {
    if (e.type == InputTypeShort) Scenes::pop();
}
static void ptick(uint32_t now) {
    if (now >= pUntil) Scenes::pop();
}
const Scene scene_popup = { "Popup", penter, nullptr, pdraw, pinput, ptick };

void popup_show(const char* title, const char* body, uint32_t ms) {
    pTitle = title; pBody = body; pUntil = millis() + ms;
    Scenes::push(&scene_popup);
}

/* --- keyboard --- */
static char* kBuf = nullptr;
static size_t kLen = 0;
static const char* kTitle = "";
static void (*kDone)(bool);
static int kRow = 0, kCol = 0;
static const char* kRows[4] = {
    "1234567890",
    "qwertyuiop",
    "asdfghjkl",
    "zxcvbnm_-"
};
static int kRowLen(int r) { return (int)strlen(kRows[r]); }

static void kenter() { kRow = 1; kCol = 0; }
static void kdraw(Canvas& c) {
    c.clear(0);
    app_header(c, kTitle);
    c.rect(2, 13, 124, 12);
    c.text(5, 15, kBuf ? kBuf : "", &tf_primary);
    int cx = 5 + c.textWidth(kBuf ? kBuf : "", &tf_primary);
    if ((millis() / 400) & 1) c.vline(cx, 15, 8);
    for (int r = 0; r < 4; r++) {
        int n = kRowLen(r);
        int x = 2;
        int y = 28 + r * 9;
        for (int i = 0; i < n; i++) {
            char ch[2] = { kRows[r][i], 0 };
            bool on = (r == kRow && i == kCol);
            if (on) {
                c.fill(x, y, 11, 9);
                c.setColor(0);
                c.text(x + 3, y + 1, ch, &tf_primary);
                c.setColor(1);
            } else {
                c.rect(x, y, 11, 9);
                c.text(x + 3, y + 1, ch, &tf_primary);
            }
            x += 12;
        }
    }
}
static void kinput(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeLong) {
        if (kDone) kDone(false);
        Scenes::pop();
        return;
    }
    if (e.type != InputTypeShort && e.type != InputTypeRepeat) return;
    if (e.key == InputKeyUp) { kRow = (kRow + 3) % 4; if (kCol >= kRowLen(kRow)) kCol = kRowLen(kRow) - 1; gCanvas.markDirty(); }
    if (e.key == InputKeyDown) { kRow = (kRow + 1) % 4; if (kCol >= kRowLen(kRow)) kCol = kRowLen(kRow) - 1; gCanvas.markDirty(); }
    if (e.key == InputKeyLeft || (e.key == InputKeyUp && false)) {}
    /* encoder is up/down; use long-ok as backspace, ok as insert, back as cancel */
    if (e.key == InputKeyOk) {
        size_t n = kBuf ? strlen(kBuf) : 0;
        if (n + 1 < kLen) {
            kBuf[n] = kRows[kRow][kCol];
            kBuf[n + 1] = 0;
        }
        gCanvas.markDirty();
    }
    if (e.key == InputKeyBack) {
        if (kBuf && *kBuf) kBuf[strlen(kBuf) - 1] = 0;
        else { if (kDone) kDone(false); Scenes::pop(); }
        gCanvas.markDirty();
    }
}
const Scene scene_keyboard = { "Keyboard", kenter, nullptr, kdraw, kinput, nullptr };

void keyboard_show(const char* title, char* buf, size_t buflen, void (*done)(bool ok)) {
    kTitle = title; kBuf = buf; kLen = buflen; kDone = done;
    Scenes::push(&scene_keyboard);
}
