#include "../scene.h"
#include "../gui/widgets.h"
#include "../gui/assets_fonts.h"
#include "../gui/assets_icons.h"
#include "../dolphin/assets_dolphin.h"
#include "../board.h"
#include "../input.h"
#include "../led.h"

static uint8_t frame = 0;
static uint8_t anim = 0; /* 0 idle, 1 happy, 2 sleep */
static uint32_t lastF = 0;
static uint32_t lastPet = 0;

static void enter() {
    anim = 0; frame = 0; lastF = millis();
}
static void draw(Canvas& c) {
    c.clear(0);
    statusbar_draw(c, nullptr);
    const TfIcon* ic = &tfi_d_idle0;
    if (anim == 0) ic = dolphin_idle[frame % dolphin_idle_count];
    else if (anim == 1) ic = dolphin_happy[frame % dolphin_happy_count];
    else ic = dolphin_sleep[frame % dolphin_sleep_count];
    int x = (CANVAS_W - ic->w) / 2;
    int y = 12 + (CANVAS_H - 12 - ic->h) / 2;
    c.icon(x, y, ic);
    /* hint */
    c.text(2, 56, "OK menu", &tf_secondary);
    if (Board::isPlus()) c.textRight(126, 56, "Plus", &tf_secondary);
}
static void input(const InputEvent& e) {
    if (e.type != InputTypeShort && e.type != InputTypeLong) return;
    if (e.key == InputKeyOk && e.type == InputTypeShort) {
        Scenes::push(&scene_main_menu);
        return;
    }
    if (e.key == InputKeyUp || e.key == InputKeyLeft) {
        anim = (uint8_t)((anim + 2) % 3);
        frame = 0; gCanvas.markDirty();
        Led::blink(255, 90, 0, 120);
    }
    if (e.key == InputKeyDown || e.key == InputKeyRight) {
        anim = (uint8_t)((anim + 1) % 3);
        frame = 0; gCanvas.markDirty();
    }
    if (e.key == InputKeyOk && e.type == InputTypeLong) {
        anim = 1; frame = 0; lastPet = millis();
        Led::blink(255, 40, 80, 250);
        gCanvas.markDirty();
    }
}
static void tick(uint32_t now) {
    if (now - lastF > 400) {
        lastF = now;
        frame++;
        gCanvas.markDirty();
    }
    if (anim == 1 && lastPet && now - lastPet > 2500) {
        anim = 0; lastPet = 0;
    }
    if (Input::idleMs() > 60000) anim = 2;
}
const Scene scene_desktop = { "Desktop", enter, nullptr, draw, input, tick };
