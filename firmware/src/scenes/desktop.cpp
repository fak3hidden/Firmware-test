#include "../scene.h"
#include "../gui/elements.h"
#include "../gui/assets_fonts.h"
#include "../dolphin/assets_dolphin.h"
#include "../board.h"
#include "../input.h"
#include "../led.h"
#include "../ble.h"

static uint8_t frame = 0;
static uint8_t anim = 0; /* 0 idle, 1 happy, 2 sleep */
static uint32_t lastF = 0;
static uint32_t lastPet = 0;

static void enter() {
    anim = 0; frame = 0; lastF = millis();
}
static void draw(Canvas& c) {
    c.clear(0);
    statusbar_draw(c);
    const TfIcon* ic = dolphin_idle[frame % dolphin_idle_count];
    if (anim == 1) ic = dolphin_happy[frame % dolphin_happy_count];
    else if (anim == 2) ic = dolphin_sleep[frame % dolphin_sleep_count];
    /* Desktop window starts at y=13. Centre the mascot in the remaining 51px. */
    int x = 8;
    int y = 13 + (51 - ic->h) / 2;
    if (y < 13) y = 13;
    c.setColor(1);
    c.icon(x, y, ic);
}
static void input(const InputEvent& e) {
    if (e.type != InputTypeShort && e.type != InputTypeLong) return;
    if (e.key == InputKeyOk && e.type == InputTypeShort) {
        Scenes::push(&scene_main_menu);
        return;
    }
    if (e.key == InputKeyDown && e.type == InputTypeShort) {
        Scenes::push(&scene_archive);
        return;
    }
    if (e.key == InputKeyUp && e.type == InputTypeShort) {
        Scenes::push(&scene_passport);
        return;
    }
    if (e.key == InputKeyLeft || e.key == InputKeyRight) {
        anim = (uint8_t)((anim + 1) % 3);
        frame = 0;
        Led::blink(255, 90, 0, 80);
        gCanvas.markDirty();
    }
    if (e.key == InputKeyOk && e.type == InputTypeLong) {
        anim = 1; frame = 0; lastPet = millis();
        Led::blink(255, 40, 80, 200);
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
