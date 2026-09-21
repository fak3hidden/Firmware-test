#include "../scene.h"
#include "../gui/widgets.h"
#include "../gui/assets_menu.h"
#include "../board.h"

static Menu menu;

static const MenuItem kItems[] = {
    { "Sub-GHz",      &tfi_m_subghz,    &scene_subghz,    nullptr },
    { "125 kHz RFID", &tfi_m_rfid,      &scene_rfid,      nullptr },
    { "NFC",          &tfi_m_nfc,       &scene_nfc,       nullptr },
    { "Infrared",     &tfi_m_infrared,  &scene_infrared,  nullptr },
    { "GPIO",         &tfi_m_gpio,      &scene_gpio,      nullptr },
    { "iButton",      &tfi_m_ibutton,   &scene_ibutton,   nullptr },
    { "Bad USB",      &tfi_m_badusb,    &scene_badusb,    nullptr },
    { "U2F",          &tfi_m_u2f,       &scene_u2f,       nullptr },
    { "Bluetooth",    &tfi_m_bt,        &scene_bluetooth, nullptr },
    { "nRF24",        &tfi_m_nrf24,     &scene_nrf24,     nullptr },
    { "Wi-Fi",        &tfi_m_wifi,      &scene_wifi,      nullptr },
    { "Applications", &tfi_m_apps,      &scene_apps,      nullptr },
    { "Archive",      &tfi_m_archive,   &scene_archive,   nullptr },
    { "Passport",     &tfi_m_passport,  &scene_passport,  nullptr },
    { "Settings",     &tfi_m_settings,  &scene_settings,  nullptr },
};

static void enter() {
    menu.set("", kItems, (int)(sizeof(kItems) / sizeof(kItems[0])));
    menu.show_icon = true;
}
static void draw(Canvas& c) { menu.draw(c); }
static void input(const InputEvent& e) {
    if (e.key == InputKeyBack && (e.type == InputTypeShort || e.type == InputTypeLong)) {
        Scenes::pop();
        return;
    }
    if (menu.input(e)) return;
    if (e.key == InputKeyOk && e.type == InputTypeShort) {
        const MenuItem& it = kItems[menu.selected()];
        if (it.scene) Scenes::push(it.scene);
        if (it.enter) it.enter();
    }
}
const Scene scene_main_menu = { "Main Menu", enter, nullptr, draw, input, nullptr };
