#pragma once
#include <stdint.h>

namespace Protocol {
    void init();
    void poll();
    bool hostConnected();
}
