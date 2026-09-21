#include "../scene.h"
#include "../gui/widgets.h"
#include "../gui/assets_icons.h"
#include "../board.h"

static Menu menu;

static const MenuItem kItems[] = {
    { "Sub-GHz",      &tfi_subghz,    &scene_subghz,    nullptr },
    { "125 kHz RFID", &tfi_rfid125,   &scene_rfid,      nullptr },
    { "NFC",          &tfi_nfc,       &scene_nfc,       nullptr },
    { "Infrared",     &tfi_infrared,  &scene_infrared,  nullptr },
    { "GPIO",         &tfi_gpio,      &scene_gpio,      nullptr },
    { "iButton",      &tfi_ibutton,   &scene_ibutton,   nullptr },
    { "Bad USB",      &tfi_badusb,    &scene_badusb,    nullptr },
    { "U2F",          &tfi_u2f,       &scene_u2f,       nullptr },
    { "nRF24",        &tfi_nrf24,     &scene_nrf24,     nullptr },
    { "Wi-Fi",        &tfi_wifi,      &scene_wifi,      nullptr },
    { "Applications", &tfi_folder,    &scene_apps,      nullptr },
    { "Archive",      &tfi_filedoc,   &scene_archive,   nullptr },
    { "Settings",     &tfi_settings,  &scene_settings,  nullptr },
};

static void enter() {
    menu.set("Main Menu", kItems, (int)(sizeof(kItems) / sizeof(kItems[0])));
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
