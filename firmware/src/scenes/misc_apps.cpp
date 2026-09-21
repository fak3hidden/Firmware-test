#include "../scene.h"
#include "../gui/widgets.h"
#include "../gui/elements.h"
#include "../gui/assets_fonts.h"
#include "../gui/assets_icons.h"
#include "../board.h"
#include "../led.h"
#include "../appvm.h"
#include "../storage.h"
#include "../display.h"
#include "../drivers/nrf24.h"
#include "../version.h"
#include <WiFi.h>
#include <Preferences.h>
#if __has_include(<USBHIDKeyboard.h>)
#include <USB.h>
#include <USBHIDKeyboard.h>
#define FINOS_HID 1
#endif

/* ---------- 125 kHz RFID (no hardware) ---------- */
static void r_enter() {}
static void r_draw(Canvas& c) {
    c.clear(0);
    c.setColor(1);
    c.str(4, 11, "125 kHz RFID", &tf_primary);
    c.str(4, 28, "No 125 kHz radio on this", &tf_secondary);
    c.str(4, 38, "hardware. Use NFC (13.56).", &tf_secondary);
    elements_button_left(c, "Back");
}
static void r_in(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) Scenes::pop();
}
const Scene scene_rfid = { "RFID", r_enter, nullptr, r_draw, r_in, nullptr };

/* ---------- iButton ---------- */
static void ib_draw(Canvas& c) {
    c.clear(0); app_header(c, "iButton");
    c.text(6, 18, "No 1-Wire probe", &tf_primary_bold);
    c.text(6, 30, "wired on this board.", &tf_primary);
    c.text(6, 42, "Use GPIO to bitbang.", &tf_primary);
}
const Scene scene_ibutton = { "iButton", r_enter, nullptr, ib_draw, r_in, nullptr };

/* ---------- U2F ---------- */
static void u_draw(Canvas& c) {
    c.clear(0); app_header(c, "U2F");
    c.text(6, 20, "U2F / FIDO is not", &tf_primary);
    c.text(6, 30, "enabled in this", &tf_primary);
    c.text(6, 40, "build.", &tf_primary);
}
const Scene scene_u2f = { "U2F", r_enter, nullptr, u_draw, r_in, nullptr };

/* ---------- GPIO ---------- */
static int gPin = 0;
static const struct { const char* n; int pin; } gPins[] = {
    { "SDA  IO8", 8 }, { "SCL IO18", 18 },
    { "TX  IO43", 43 }, { "RX  IO44", 44 },
    { "IR TX IO2", 2 }, { "IR RX IO1", 1 },
};
static bool gLevel[6];
static int gLedHue = 0;
static Menu gMenu;
static const MenuItem gItems[] = {
    { "Pin out", nullptr, nullptr, nullptr },
    { "RGB LEDs", nullptr, nullptr, nullptr },
    { "USB-UART", nullptr, nullptr, nullptr },
};
static int gMode = 0;
static void gpio_enter() {
    gMode = 0; gMenu.set("GPIO", gItems, 3); gMenu.show_icon = false;
    for (int i = 0; i < 6; i++) {
        pinMode(gPins[i].pin, INPUT);
        gLevel[i] = digitalRead(gPins[i].pin);
    }
}
static void gpio_draw(Canvas& c) {
    if (gMode == 0) { gMenu.draw(c); return; }
    c.clear(0);
    if (gMode == 1) {
        app_header(c, "GPIO");
        for (int i = 0; i < 6; i++) {
            int y = 13 + i * 8;
            if (i == gPin) { c.rbox(1, y - 1, 126, 8, 2); c.setColor(0); }
            c.text(4, y, gPins[i].n, &tf_primary);
            c.textRight(124, y, gLevel[i] ? "1" : "0", &tf_primary);
            c.setColor(1);
        }
    } else if (gMode == 2) {
        app_header(c, "RGB LEDs");
        c.text(8, 20, "Encoder: hue", &tf_primary);
        char b[16]; snprintf(b, sizeof(b), "H=%d", gLedHue);
        c.text(8, 32, b, &tf_big);
        c.text(8, 52, "OK apply  Back", &tf_secondary);
    }
}
static void hsv(int h, uint8_t& r, uint8_t& g, uint8_t& b) {
    int x = h % 360; if (x < 0) x += 360;
    int c = 255, m = 0;
    int sext = x / 60;
    int f = (x % 60) * 255 / 60;
    switch (sext) {
        case 0: r = c; g = f; b = m; break;
        case 1: r = 255 - f; g = c; b = m; break;
        case 2: r = m; g = c; b = f; break;
        case 3: r = m; g = 255 - f; b = c; break;
        case 4: r = f; g = m; b = c; break;
        default: r = c; g = m; b = 255 - f; break;
    }
    r /= 8; g /= 8; b /= 8; /* dim */
}
static void gpio_in(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) {
        if (gMode) { gMode = 0; Led::off(); gCanvas.markDirty(); return; }
        Scenes::pop(); return;
    }
    if (gMode == 0) {
        if (gMenu.input(e)) return;
        if (e.key == InputKeyOk && e.type == InputTypeShort) {
            int s = gMenu.selected();
            if (s == 0) gMode = 1;
            else if (s == 1) gMode = 2;
            else popup_show("USB-UART", "CDC already on", 800);
            gCanvas.markDirty();
        }
        return;
    }
    if (gMode == 1) {
        if (e.key == InputKeyUp) gPin = (gPin + 5) % 6;
        if (e.key == InputKeyDown) gPin = (gPin + 1) % 6;
        if (e.key == InputKeyOk && e.type == InputTypeShort) {
            pinMode(gPins[gPin].pin, OUTPUT);
            gLevel[gPin] = !gLevel[gPin];
            digitalWrite(gPins[gPin].pin, gLevel[gPin] ? HIGH : LOW);
        }
        gCanvas.markDirty();
    }
    if (gMode == 2) {
        if (e.key == InputKeyUp) gLedHue = (gLedHue + 330) % 360;
        if (e.key == InputKeyDown) gLedHue = (gLedHue + 30) % 360;
        uint8_t r, g, b; hsv(gLedHue, r, g, b);
        Led::fill(r, g, b); Led::show();
        gCanvas.markDirty();
    }
}
const Scene scene_gpio = { "GPIO", gpio_enter, nullptr, gpio_draw, gpio_in, nullptr };

/* ---------- Bad USB ---------- */
#if FINOS_HID
static USBHIDKeyboard Keyboard;
static bool hidOn = false;
#endif
static char script[128] = "hello from finos";
static Menu bMenu;
static const MenuItem bItems[] = {
    { "Type demo text", nullptr, nullptr, nullptr },
    { "Edit text", nullptr, nullptr, nullptr },
    { "Load from file", nullptr, nullptr, nullptr },
};
static void hid_ensure() {
#if FINOS_HID
    if (!hidOn) { USB.begin(); Keyboard.begin(); hidOn = true; }
#endif
}
static void bad_enter() { bMenu.set("Bad USB", bItems, 3); bMenu.show_icon = false; }
static void bad_draw(Canvas& c) { bMenu.draw(c); }
static void kdone(bool ok) { (void)ok; }
static void bad_in(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) { Scenes::pop(); return; }
    if (bMenu.input(e)) return;
    if (e.key == InputKeyOk && e.type == InputTypeShort) {
        int s = bMenu.selected();
        if (s == 0) {
            hid_ensure();
#if FINOS_HID
            delay(400);
            Keyboard.print(script);
            popup_show("Bad USB", "Typed", 700);
#else
            popup_show("Bad USB", "HID not in this build", 900);
#endif
        } else if (s == 1) {
            keyboard_show("Demo text", script, sizeof(script), kdone);
        } else {
            popup_show("Bad USB", "Put txt in /ext/badusb", 900);
        }
    }
}
const Scene scene_badusb = { "Bad USB", bad_enter, nullptr, bad_draw, bad_in, nullptr };

/* ---------- nRF24 ---------- */
static Menu nMenu;
static uint8_t hits[126];
static int nMode = 0;
static const MenuItem nItems[] = {
    { "Channel scanner", nullptr, nullptr, nullptr },
    { "Set channel", nullptr, nullptr, nullptr },
    { "Info", nullptr, nullptr, nullptr },
};
static void n_enter() {
    nMode = 0; nMenu.set("nRF24", nItems, 3); nMenu.show_icon = false;
    if (!NRF24::present()) NRF24::init();
}
static void n_draw(Canvas& c) {
    if (nMode == 0) { nMenu.draw(c); return; }
    c.clear(0); app_header(c, "nRF24 Scan");
    /* 126 channels -> 126 px wide sparkline in 120 px */
    for (int i = 0; i < 120; i++) {
        int h = hits[i] ? (hits[i] * 6 + 2) : 0;
        if (h > 36) h = 36;
        if (h) c.vline(4 + i, 54 - h, h);
    }
    c.text(4, 56, "2.4 GHz  ch 0-125", &tf_secondary);
}
static void n_in(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) {
        if (nMode) { nMode = 0; gCanvas.markDirty(); return; }
        Scenes::pop(); return;
    }
    if (nMode == 0) {
        if (nMenu.input(e)) return;
        if (e.key == InputKeyOk && e.type == InputTypeShort) {
            if (!NRF24::present()) { popup_show("nRF24", "Not found (Plus?)", 900); return; }
            int s = nMenu.selected();
            if (s == 0) { nMode = 1; NRF24::scan(hits, 2); }
            else if (s == 1) popup_show("Channel", "0 (default)", 700);
            else popup_show("nRF24", Board::isPlus() ? "Plus module OK" : "Optional module", 900);
            gCanvas.markDirty();
        }
    }
}
const Scene scene_nrf24 = { "nRF24", n_enter, nullptr, n_draw, n_in, nullptr };

/* ---------- Wi-Fi ---------- */
static Menu wMenu;
static int wMode = 0;
static int wCount = 0;
static int wSel = 0;
static char wSSID[12][24];
static int wRSSI[12];
static const MenuItem wItems[] = {
    { "Scan APs", nullptr, nullptr, nullptr },
    { "Disconnect", nullptr, nullptr, nullptr },
};
static void w_enter() { wMode = 0; wMenu.set("Wi-Fi", wItems, 2); wMenu.show_icon = false; }
static void w_draw(Canvas& c) {
    if (wMode == 0) { wMenu.draw(c); return; }
    c.clear(0); app_header(c, "Wi-Fi Scan");
    if (wCount == 0) { c.text(8, 28, "No APs", &tf_primary); return; }
    for (int i = 0; i < wCount && i < 5; i++) {
        int y = 14 + i * 9;
        if (i == wSel) { c.rbox(1, y - 1, 126, 9, 2); c.setColor(0); }
        c.text(4, y, wSSID[i], &tf_primary);
        char b[8]; snprintf(b, sizeof(b), "%d", wRSSI[i]);
        c.textRight(124, y, b, &tf_secondary);
        c.setColor(1);
    }
}
static void w_in(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) {
        if (wMode) { wMode = 0; WiFi.scanDelete(); gCanvas.markDirty(); return; }
        Scenes::pop(); return;
    }
    if (wMode == 0) {
        if (wMenu.input(e)) return;
        if (e.key == InputKeyOk && e.type == InputTypeShort) {
            if (wMenu.selected() == 0) {
                popup_show("Wi-Fi", "Scanning...", 400);
                WiFi.mode(WIFI_STA);
                WiFi.disconnect();
                int n = WiFi.scanNetworks();
                wCount = n > 12 ? 12 : n;
                for (int i = 0; i < wCount; i++) {
                    strncpy(wSSID[i], WiFi.SSID(i).c_str(), 23); wSSID[i][23] = 0;
                    wRSSI[i] = WiFi.RSSI(i);
                }
                wSel = 0; wMode = 1;
            } else { WiFi.disconnect(true); popup_show("Wi-Fi", "Idle", 600); }
            gCanvas.markDirty();
        }
        return;
    }
    if (e.key == InputKeyUp && wCount) wSel = (wSel + wCount - 1) % wCount;
    if (e.key == InputKeyDown && wCount) wSel = (wSel + 1) % wCount;
    gCanvas.markDirty();
}
const Scene scene_wifi = { "Wi-Fi", w_enter, nullptr, w_draw, w_in, nullptr };

/* ---------- Applications ---------- */
static int aSel = 0;
static void a_enter() { AppVM::init(); aSel = 0; }
static void a_draw(Canvas& c) {
    c.clear(0); app_header(c, "Applications");
    int n = AppVM::count();
    if (n == 0) {
        c.text(8, 22, "No apps installed.", &tf_primary);
        c.text(8, 34, "Use the App Store", &tf_primary);
        c.text(8, 44, "or qFin to install.", &tf_secondary);
        return;
    }
    for (int i = 0; i < n && i < 5; i++) {
        char nm[24]; AppVM::nameAt(i, nm, sizeof(nm));
        int y = 14 + i * 9;
        if (i == aSel) { c.rbox(1, y - 1, 126, 9, 2); c.setColor(0); }
        c.text(6, y, nm, &tf_primary);
        c.setColor(1);
    }
}
static void a_in(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) { Scenes::pop(); return; }
    int n = AppVM::count();
    if (!n) return;
    if (e.key == InputKeyUp) aSel = (aSel + n - 1) % n;
    if (e.key == InputKeyDown) aSel = (aSel + 1) % n;
    if (e.key == InputKeyOk && e.type == InputTypeShort) AppVM::run(aSel);
    gCanvas.markDirty();
}
const Scene scene_apps = { "Apps", a_enter, nullptr, a_draw, a_in, nullptr };

/* ---------- Archive ---------- */
static const char* roots[] = { "/ext/subghz", "/ext/nfc", "/ext/infrared", "/ext/apps", "/ext/badusb" };
static const char* rlab[] = { "Sub-GHz", "NFC", "Infrared", "Apps", "Bad USB" };
static int rSel = 0, rMode = 0, fSel = 0, fN = 0;
static char fNames[16][32];
static void ar_enter() { rMode = 0; rSel = 0; }
static void ar_draw(Canvas& c) {
    c.clear(0); app_header(c, "Archive");
    if (rMode == 0) {
        for (int i = 0; i < 5; i++) {
            int y = 14 + i * 9;
            if (i == rSel) { c.rbox(1, y - 1, 126, 9, 2); c.setColor(0); }
            c.text(6, y, rlab[i], &tf_primary);
            c.setColor(1);
        }
    } else {
        if (fN == 0) { c.text(8, 28, "Empty", &tf_primary); return; }
        for (int i = 0; i < fN && i < 5; i++) {
            int y = 14 + i * 9;
            if (i == fSel) { c.rbox(1, y - 1, 126, 9, 2); c.setColor(0); }
            c.text(6, y, fNames[i], &tf_primary);
            c.setColor(1);
        }
    }
}
static void ar_in(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) {
        if (rMode) { rMode = 0; gCanvas.markDirty(); return; }
        Scenes::pop(); return;
    }
    if (rMode == 0) {
        if (e.key == InputKeyUp) rSel = (rSel + 4) % 5;
        if (e.key == InputKeyDown) rSel = (rSel + 1) % 5;
        if (e.key == InputKeyOk && e.type == InputTypeShort) {
            fN = Storage::list(roots[rSel], fNames, 16, false);
            fSel = 0; rMode = 1;
        }
    } else {
        if (e.key == InputKeyUp && fN) fSel = (fSel + fN - 1) % fN;
        if (e.key == InputKeyDown && fN) fSel = (fSel + 1) % fN;
        if (e.key == InputKeyOk && fN && e.type == InputTypeLong) {
            String p = String(roots[rSel]) + "/" + fNames[fSel];
            Storage::remove(p.c_str());
            fN = Storage::list(roots[rSel], fNames, 16, false);
            popup_show("Deleted", fNames[fSel], 700);
        }
    }
    gCanvas.markDirty();
}
const Scene scene_archive = { "Archive", ar_enter, nullptr, ar_draw, ar_in, nullptr };

/* ---------- Settings ---------- */
static VarList vars;
static VarList::Item vitems[6];
static int bright = 255, rot = 3, sleepm = 0;
static Preferences prefs;

static void ch_bright(int dir, VarList::Item* it) {
    bright += dir * 32;
    if (bright < 32) bright = 32;
    if (bright > 255) bright = 255;
    Board::setBacklight((uint8_t)bright);
    snprintf(it->value, sizeof(it->value), "%d", bright);
    prefs.putInt("bright", bright);
}
static void ch_rot(int, VarList::Item* it) {
    rot = (rot == 3) ? 1 : 3;
    Display::setRotation((uint8_t)rot);
    Display::fillBezel();
    snprintf(it->value, sizeof(it->value), "%d", rot);
    prefs.putInt("rot", rot);
    gCanvas.markDirty();
}
static void ch_sleep(int dir, VarList::Item* it) {
    static const int mins[] = {0, 1, 2, 5, 10};
    static const char* lab[] = {"Off", "1 min", "2 min", "5 min", "10 min"};
    sleepm = (sleepm + dir + 5) % 5;
    snprintf(it->value, sizeof(it->value), "%s", lab[sleepm]);
    prefs.putInt("sleep", sleepm);
    (void)mins;
}
static void set_enter() {
    prefs.begin("finos", false);
    bright = prefs.getInt("bright", 255);
    rot = prefs.getInt("rot", 3);
    sleepm = prefs.getInt("sleep", 0);
    vitems[0] = {"LCD brightness", "", ch_bright};
    vitems[1] = {"Rotation", "", ch_rot};
    vitems[2] = {"Auto sleep", "", ch_sleep};
    vitems[3] = {"About", ">", nullptr};
    snprintf(vitems[0].value, 24, "%d", bright);
    snprintf(vitems[1].value, 24, "%d", rot);
    const char* lab[] = {"Off", "1 min", "2 min", "5 min", "10 min"};
    snprintf(vitems[2].value, 24, "%s", lab[sleepm]);
    snprintf(vitems[3].value, 24, ">");
    vars.title = "Settings";
    vars.items = vitems;
    vars.count = 4;
    vars.sel = 0;
}
static void set_draw(Canvas& c) { vars.draw(c); }
static void set_in(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) { Scenes::pop(); return; }
    if (vars.input(e)) return;
    if (e.key == InputKeyOk && e.type == InputTypeShort && vars.sel == 3)
        Scenes::push(&scene_about);
}
const Scene scene_settings = { "Settings", set_enter, nullptr, set_draw, set_in, nullptr };

/* ---------- About ---------- */
static void ab_draw(Canvas& c) {
    c.clear(0); app_header(c, "About");
    c.text(6, 16, FINOS_NAME " " FINOS_VERSION, &tf_primary_bold);
    c.text(6, 28, FINOS_BOARD_NAME, &tf_primary);
    c.text(6, 38, "128x64 Flipper UI", &tf_secondary);
    c.text(6, 48, "Fin the dolphin", &tf_secondary);
    c.text(6, 56, FINOS_BUILD_DATE, &tf_secondary);
}
const Scene scene_about = { "About", r_enter, nullptr, ab_draw, r_in, nullptr };

/* ---------- Bluetooth ---------- */
#include "../ble.h"
static Menu btMenu;
static const MenuItem btItems[] = {
    { "Toggle", nullptr, nullptr, nullptr },
    { "Forget pairing", nullptr, nullptr, nullptr },
    { "Name", nullptr, nullptr, nullptr },
};
static void bt_enter() {
    btMenu.set("Bluetooth", btItems, 3);
    btMenu.show_icon = false;
}
static void bt_draw(Canvas& c) { btMenu.draw(c); }
static void bt_in(const InputEvent& e) {
    if (e.key == InputKeyBack && e.type == InputTypeShort) { Scenes::pop(); return; }
    if (btMenu.input(e)) return;
    if (e.key == InputKeyOk && e.type == InputTypeShort) {
        int s = btMenu.selected();
        if (s == 0) {
            Ble::enable(!Ble::enabled());
            popup_show("Bluetooth", Ble::enabled() ? "On" : "Off", 700);
        } else if (s == 1) {
            Ble::forget();
            popup_show("Bluetooth", "Advertising", 700);
        } else {
            popup_show(Ble::name(), Ble::connected() ? "Connected" : "Idle", 900);
        }
    }
}
const Scene scene_bluetooth = { "Bluetooth", bt_enter, nullptr, bt_draw, bt_in, nullptr };

/* ---------- Passport ---------- */
#include "../dolphin/assets_dolphin.h"
static void pass_draw(Canvas& c) {
    c.clear(0);
    c.setColor(1);
    c.icon(0, 10, &tfi_d_idle0);
    c.str(60, 16, "Passport", &tf_primary);
    c.str(60, 28, "Flipper", &tf_secondary);
    c.str(60, 38, "Level 1", &tf_secondary);
    c.str(60, 48, Ble::connected() ? "BT: yes" : "BT: no", &tf_secondary);
}
const Scene scene_passport = { "Passport", r_enter, nullptr, pass_draw, r_in, nullptr };
