#pragma once
#include <stdint.h>
#include <stddef.h>

namespace NFC {
    bool init();
    bool present();
    /* Poll ISO14443A. Returns UID length (4/7/10) or 0. */
    uint8_t pollA(uint8_t* uid, uint8_t maxuid, uint16_t* atqa = nullptr, uint8_t* sak = nullptr);
    bool readNdef(char* out, size_t maxn);
    void sleep();
}
