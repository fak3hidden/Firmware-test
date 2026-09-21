#pragma once
#include <stdint.h>
#include <stddef.h>

namespace Ble {
    void init();
    void enable(bool on);
    bool enabled();
    bool connected();
    void setName(const char* name);
    const char* name();
    void forget();
    void poll();
    /* Injected input from phone/PC (returns true if an event was queued). */
    bool popRemote(uint8_t& key, uint8_t& type);
    void send(const uint8_t* data, size_t n);
}
