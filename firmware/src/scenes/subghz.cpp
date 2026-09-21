#include "../scene.h"
#include "../gui/widgets.h"
#include "../gui/elements.h"
#include "../gui/assets_fonts.h"
#include "../drivers/cc1101.h"
#include "../storage.h"
#include "../led.h"
#include "../board.h"

static Menu menu;
static int mode = 0; /* 0 menu, 1 read, 2 analyzer, 3 saved, 4 tx */
static float freqs[] = {315.0f, 433.92f, 868.0f, 915.0f};
static const char* flab[] = {"315.00", "433.92", "868.00", "915.00"};
static int fi = 1;
static uint16_t cap[256];
static int capn = 0;
static uint32_t lastRx = 0;
static int rssi = -99;
static char names[16][32];
static int nfiles = 0, fsel = 0;
static char msg[32];

static const MenuItem kItems[] = {
    { "Read", nullptr, nullptr, nullptr },
    { "Read RAW", nullptr, nullptr, nullptr },
    { "Frequency Analyzer", nullptr, nullptr, nullptr },
    { "Saved", nullptr, nullptr, nullptr },
    { "Transmit", nullptr, nullptr, nullptr },
    { "Frequency", nullptr, nullptr, nullptr },
};

static void setFreq() {
    CC1101::setFrequency(freqs[fi]);
    snprintf(msg, sizeof(msg), "%s MHz", flab[fi]);
}

static void enter() {
    mode = 0;
    menu.set("Sub-GHz", kItems, 6);
    menu.show_icon = false;
    if (!CC1101::present()) CC1101::init();
    setFreq();
}
static void exit() {
    CC1101::stopSniff();
    CC1101::idle();
}

static void drawRead(Canvas& c, bool raw) {
    c.clear(0);
    app_header(c, raw ? "Sub-GHz RAW" : "Sub-GHz Read");
    c.text(4, 16, flab[fi], &tf_big);
    c.text(70, 22, "MHz", &tf_primary);
    c.text(4, 36, "Listening...", &tf_primary);
    char b[24];
    snprintf(b, sizeof(b), "RSSI %d dBm", rssi);
    c.text(4, 46, b, &tf_secondary);
    snprintf(b, sizeof(b), "pulses %d", capn);
    c.text(4, 46, b, &tf_secondary);
    elements_button_left(c, "Back");
    elements_button_right(c, "Save");
}

static void drawAnalyzer(Canvas& c) {
    c.clear(0);
    app_header(c, "Freq Analyzer");
    /* hop display */
    c.text(4, 16, flab[fi], &tf_big);
    c.text(70, 22, "MHz", &tf_primary);
    int h = rssi + 100;
    if (h < 0) h = 0;
    if (h > 40) h = 40;
    c.fill(4, 60 - h, 120, h);
    c.text(4, 54, "Rotate to hop", &tf_secondary);
}

static void drawSaved(Canvas& c) {
    c.clear(0);
    app_header(c, "Saved");
    if (nfiles == 0) {
        c.text(8, 28, "No captures", &tf_primary);
        return;
    }
    for (int i = 0; i < nfiles && i < 5; i++) {
        int y = 14 + i * 9;
        if (i == fsel) {
            c.rbox(2, y - 1, 124, 9, 2);
            c.setColor(0);
            c.text(6, y, names[i], &tf_primary);
            c.setColor(1);
        } else c.text(6, y, names[i], &tf_primary);
    }
}

static void draw(Canvas& c) {
    if (mode == 0) menu.draw(c);
    else if (mode == 1 || mode == 2) drawRead(c, mode == 2);
    else if (mode == 3) drawAnalyzer(c);
    else if (mode == 4) drawSaved(c);
}

static void startRead() {
    capn = 0;
    CC1101::startSniff();
    lastRx = millis();
}

static void saveCapture() {
    if (capn < 4) { popup_show("Sub-GHz", "Nothing to save", 900); return; }
    char path[48];
    snprintf(path, sizeof(path), "/ext/subghz/raw_%lu.sub", (unsigned long)(millis() / 1000));
    String s = "Filetype: FinOS SubGhz RAW File\nVersion: 1\nFrequency: ";
    s += String((int)(freqs[fi] * 1000) * 1000);
    s += "\nPreset: FuriHalSubGhzPresetOok650Async\nProtocol: RAW\nRAW_Data:";
    for (int i = 0; i < capn; i++) {
        s += (i % 2 == 0) ? " " : " -";
        s += String(cap[i]);
    }
    s += "\n";
    Storage::writeText(path, s.c_str());
    popup_show("Saved", path + 12, 1200);
}

static void input(const InputEvent& e) {
    if (e.key == InputKeyBack && (e.type == InputTypeShort || e.type == InputTypeLong)) {
        if (mode != 0) {
            CC1101::stopSniff();
            mode = 0; gCanvas.markDirty(); return;
        }
        Scenes::pop(); return;
    }
    if (mode == 0) {
        if (menu.input(e)) return;
        if (e.key == InputKeyOk && e.type == InputTypeShort) {
            int s = menu.selected();
            if (s == 0) { mode = 1; startRead(); }
            else if (s == 1) { mode = 2; startRead(); }
            else if (s == 2) { mode = 3; CC1101::stopSniff(); CC1101::setFrequency(freqs[fi]); }
            else if (s == 3) {
                nfiles = Storage::list("/ext/subghz", names, 16, false);
                fsel = 0; mode = 4;
            } else if (s == 4) {
                if (capn > 2) {
                    CC1101::txRaw(cap, capn, freqs[fi]);
                    Led::blink(255, 40, 0, 200);
                    popup_show("Sub-GHz", "Transmitted", 800);
                } else popup_show("Sub-GHz", "Capture first", 800);
            } else if (s == 5) {
                fi = (fi + 1) % 4; setFreq();
                popup_show("Frequency", flab[fi], 700);
            }
            gCanvas.markDirty();
        }
        return;
    }
    if ((mode == 1 || mode == 2) && e.key == InputKeyOk && e.type == InputTypeShort) {
        CC1101::stopSniff();
        saveCapture();
        return;
    }
    if (mode == 3 && (e.key == InputKeyUp || e.key == InputKeyDown) && e.type == InputTypeShort) {
        fi = (fi + (e.key == InputKeyDown ? 1 : 3)) % 4;
        setFreq(); gCanvas.markDirty();
    }
    if (mode == 4) {
        if (e.key == InputKeyUp && nfiles) { fsel = (fsel + nfiles - 1) % nfiles; gCanvas.markDirty(); }
        if (e.key == InputKeyDown && nfiles) { fsel = (fsel + 1) % nfiles; gCanvas.markDirty(); }
        if (e.key == InputKeyOk && e.type == InputTypeShort && nfiles) {
            popup_show("Replay", names[fsel], 800);
            /* crude: re-tx last capture */
            if (capn > 2) CC1101::txRaw(cap, capn, freqs[fi]);
        }
    }
}

static void tick(uint32_t now) {
    if (mode == 1 || mode == 2) {
        uint32_t us; bool lvl;
        while (CC1101::popPulse(us, lvl) && capn < 255) {
            cap[capn++] = (uint16_t)us;
            lastRx = now;
        }
        if (now % 200 < 30) {
            rssi = CC1101::rssi();
            gCanvas.markDirty();
        }
    }
    if (mode == 3 && now % 150 < 30) {
        rssi = CC1101::rssi();
        gCanvas.markDirty();
    }
}

const Scene scene_subghz = { "Sub-GHz", enter, exit, draw, input, tick };
