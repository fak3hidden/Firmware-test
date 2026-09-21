#pragma once
#include <Arduino.h>
#include "canvas.h"
#include "input.h"

struct Scene {
    const char* name;
    void (*enter)();
    void (*exit)();
    void (*draw)(Canvas& c);
    void (*input)(const InputEvent& e);
    void (*tick)(uint32_t now);
};

namespace Scenes {
    void init();
    void push(const Scene* s);
    void pop();
    void switchTo(const Scene* s);
    const Scene* current();
    void draw(Canvas& c);
    void input(const InputEvent& e);
    void tick(uint32_t now);
    int  depth();
}

extern const Scene scene_desktop;
extern const Scene scene_main_menu;
extern const Scene scene_subghz;
extern const Scene scene_rfid;
extern const Scene scene_nfc;
extern const Scene scene_infrared;
extern const Scene scene_gpio;
extern const Scene scene_ibutton;
extern const Scene scene_badusb;
extern const Scene scene_u2f;
extern const Scene scene_apps;
extern const Scene scene_archive;
extern const Scene scene_settings;
extern const Scene scene_about;
extern const Scene scene_nrf24;
extern const Scene scene_wifi;
extern const Scene scene_keyboard;
extern const Scene scene_dialog;
extern const Scene scene_popup;
extern const Scene scene_app_run;

/* helpers used by several scenes */
void dialog_show(const char* title, const char* body,
                 const char* left, const char* right,
                 void (*on_left)(), void (*on_right)());
void popup_show(const char* title, const char* body, uint32_t ms);
void keyboard_show(const char* title, char* buf, size_t buflen, void (*done)(bool ok));
