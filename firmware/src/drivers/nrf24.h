#pragma once
#include <stdint.h>

namespace NRF24 {
    bool init();
    bool present();
    void setChannel(uint8_t ch);
    uint8_t channel();
    /* Carrier detect scan: fill 126-byte map with CD hits. */
    void scan(uint8_t* hits /*[126]*/, uint32_t dwellMs = 2);
    bool tx(const uint8_t* data, uint8_t n);
    int  rx(uint8_t* data, uint8_t n, uint32_t timeoutMs);
}
