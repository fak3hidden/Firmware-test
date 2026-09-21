#pragma once
#include <Arduino.h>
#include "scene.h"

/* Tiny JSON-ish app runtime. Apps live in /ext/apps/<id>.tapp (plain JSON). */
namespace AppVM {
    void init();
    bool install(const uint8_t* data, size_t n);
    bool uninstall(const char* id);
    String listJson();
    int  count();
    bool nameAt(int i, char* out, size_t n);
    bool run(int i);
    bool runId(const char* id);
    void tick(uint32_t now);
    void draw(Canvas& c);
    void input(const InputEvent& e);
    bool running();
    void stop();
}
