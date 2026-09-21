#include "../scene.h"
#include "../gui/widgets.h"
#include "../gui/elements.h"
#include "../gui/assets_fonts.h"
#include "../drivers/ir.h"
#include "../storage.h"
#include "../led.h"

static Menu menu;
static int mode = 0;
static uint16_t raw[128];
static int rawn = 0;
static uint32_t nec = 0;
static bool have = false;
static bool isNec = false;

static const MenuItem kItems[] = {
    { "Learn", nullptr, nullptr, nullptr },
    { "Saved", nullptr, nullptr, nullptr },
    { "Universal Remote", nullptr, nullptr, nullptr },
    { "GPIO", nullptr, nullptr, nullptr },
};

static void enter() {
    mode = 0; have = false; rawn = 0;
    menu.set("Infrared", kItems, 4);
    menu.show_icon = false;
    IR::init();
}
static void drawLearn(Canvas& c) {
    c.clear(0);
    app_header(c, "IR Learn");
    if (!have) {
        c.text(8, 24, "Point remote", &tf_primary);
        c.text(8, 34, "and press a key", &tf_primary);
    } else {
        c.text(8, 16, isNec ? "NEC" : "RAW", &tf_primary_bold);
        if (isNec) {
            char b[24];
            snprintf(b, sizeof(b), "0x%08lX", (unsigned long)nec);
            c.text(8, 30, b, &tf_primary);
        } else {
            char b[24];
            snprintf(b, sizeof(b), "%d timings", rawn);
            c.text(8, 30, b, &tf_primary);
        }
        elements_button_right(c, "Send");
    }
    elements_button_left(c, "Back");
}
static void draw(Canvas& c) {
    if (mode == 0) menu.draw(c);
    else drawLearn(c);
}
static void input(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) {
        if (mode == 1 && have) {
            char path[48];
            snprintf(path, sizeof(path), "/ext/infrared/ir_%lu.ir", (unsigned long)(millis() / 1000));
            String s = "Filetype: IR signals file\nVersion: 1\n#\nname: learned\ntype: ";
            if (isNec) {
                s += "parsed\nprotocol: NEC\naddress: 00 00 00 00\ncommand: ";
                char b[16]; snprintf(b, sizeof(b), "%08lX", (unsigned long)nec);
                s += b; s += "\n";
            } else {
                s += "raw\nfrequency: 38000\nduty_cycle: 0.33\ndata:";
                for (int i = 0; i < rawn; i++) { s += " "; s += String(raw[i]); }
                s += "\n";
            }
            Storage::writeText(path, s.c_str());
            popup_show("Saved", "IR file", 800);
            mode = 0; return;
        }
        if (mode) { mode = 0; gCanvas.markDirty(); return; }
        Scenes::pop(); return;
    }
    if (mode == 0) {
        if (menu.input(e)) return;
        if (e.key == InputKeyOk && e.type == InputTypeShort) {
            int s = menu.selected();
            if (s == 0) { mode = 1; have = false; rawn = 0; }
            else if (s == 2) {
                /* TV power NEC 0x20DF10EF is a common example - we send a dummy only if user captured */
                popup_show("Universal", "Learn a remote first", 900);
            } else popup_show("Infrared", "38 kHz IR TX/RX", 800);
            gCanvas.markDirty();
        }
        return;
    }
    if (mode == 1 && e.key == InputKeyOk && have) {
        if (isNec) IR::sendNEC(nec);
        else IR::sendRaw(raw, rawn);
        Led::blink(255, 0, 0, 150);
        popup_show("IR", "Sent", 500);
    }
}
static void tick(uint32_t) {
    if (mode == 1 && !have) {
        int n = IR::capture(raw, 128, 50);
        if (n > 4) {
            rawn = n;
            isNec = IR::decodeNEC(raw, n, nec);
            have = true;
            Led::blink(0, 180, 40, 150);
            gCanvas.markDirty();
        }
    }
}
const Scene scene_infrared = { "Infrared", enter, nullptr, draw, input, tick };
