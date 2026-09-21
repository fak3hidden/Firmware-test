#pragma once
#include <stdint.h>

namespace Led {
    void init();
    void set(uint8_t i, uint8_t r, uint8_t g, uint8_t b);
    void fill(uint8_t r, uint8_t g, uint8_t b);
    void show();
    void off();
    void blink(uint8_t r, uint8_t g, uint8_t b, uint32_t ms);
    void tick(uint32_t now);
}
