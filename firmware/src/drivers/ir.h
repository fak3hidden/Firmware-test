#pragma once
#include <stdint.h>
#include <stddef.h>

namespace IR {
    void init();
    /* Learn: capture raw timings into buf (pairs mark/space us). Returns count. */
    int capture(uint16_t* buf, int maxn, uint32_t timeoutMs = 5000);
    bool sendNEC(uint32_t data, uint8_t nbits = 32);
    bool sendRaw(const uint16_t* buf, int n, uint16_t freqKHz = 38);
    /* decode NEC from raw, returns true and fills data */
    bool decodeNEC(const uint16_t* buf, int n, uint32_t& data);
}
