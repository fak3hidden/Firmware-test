#pragma once
#include <stdint.h>
#include <stddef.h>

namespace Protocol {
    void init();
    void poll();
    bool hostConnected();
    void fromBle(const uint8_t* data, size_t n);
}
