#include "../scene.h"
#include "../gui/widgets.h"
#include "../gui/assets_fonts.h"
#include "../drivers/pn532.h"
#include "../storage.h"
#include "../led.h"

static Menu menu;
static int mode = 0; /* 0 menu, 1 read */
static uint8_t uid[10];
static uint8_t uidlen = 0;
static char uidhex[32];
static bool got = false;
static uint32_t lastPoll = 0;

static const MenuItem kItems[] = {
    { "Read", nullptr, nullptr, nullptr },
    { "Saved", nullptr, nullptr, nullptr },
    { "Emulate UID", nullptr, nullptr, nullptr },
    { "Info", nullptr, nullptr, nullptr },
};

static void hexuid() {
    uidhex[0] = 0;
    char* p = uidhex;
    for (int i = 0; i < uidlen; i++) p += sprintf(p, "%02X", uid[i]);
}

static void enter() {
    mode = 0; got = false; uidlen = 0;
    menu.set("NFC", kItems, 4);
    menu.show_icon = false;
    if (!NFC::present()) NFC::init();
}
static void drawRead(Canvas& c) {
    c.clear(0);
    statusbar_draw(c, "NFC Read");
    if (!got) {
        c.text(8, 24, "Place card on", &tf_primary);
        c.text(8, 34, "the back...", &tf_primary);
        c.text(8, 52, "PN532 13.56 MHz", &tf_secondary);
    } else {
        c.text(8, 18, "ISO14443-A", &tf_primary_bold);
        c.text(8, 30, "UID:", &tf_primary);
        c.text(8, 40, uidhex, &tf_primary);
        c.text(8, 54, "OK to save", &tf_secondary);
    }
}
static void draw(Canvas& c) {
    if (mode == 0) menu.draw(c);
    else drawRead(c);
}
static void input(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) {
        if (mode) { mode = 0; gCanvas.markDirty(); return; }
        Scenes::pop(); return;
    }
    if (mode == 0) {
        if (menu.input(e)) return;
        if (e.key == InputKeyOk && e.type == InputTypeShort) {
            int s = menu.selected();
            if (s == 0) { mode = 1; got = false; }
            else if (s == 1) popup_show("NFC", "Open Archive", 800);
            else if (s == 2) popup_show("NFC", "Emulate coming soon", 900);
            else popup_show("NFC", "PN532 ISO14443A", 900);
            gCanvas.markDirty();
        }
        return;
    }
    if (e.key == InputKeyOk && got) {
        char path[48];
        snprintf(path, sizeof(path), "/ext/nfc/%s.nfc", uidhex);
        String s = "Filetype: FinOS NFC\nVersion: 1\nDevice type: NTAG/MIFARE\nUID: ";
        s += uidhex; s += "\n";
        Storage::writeText(path, s.c_str());
        popup_show("Saved", uidhex, 900);
    }
}
static void tick(uint32_t now) {
    if (mode == 1 && !got && now - lastPoll > 200) {
        lastPoll = now;
        uint8_t n = NFC::pollA(uid, 10);
        if (n) {
            uidlen = n; hexuid(); got = true;
            Led::blink(0, 180, 40, 200);
            gCanvas.markDirty();
        }
    }
}
const Scene scene_nfc = { "NFC", enter, nullptr, draw, input, tick };
