#pragma once
#include <stdint.h>
#include "canvas.h"

namespace Display {
    void init();
    void setRotation(uint8_t rot); /* 1 or 3 = landscape 320x170 */
    uint8_t rotation();
    void fillBezel();
    void present(const Canvas& c);
    void presentForce(const Canvas& c);
    int panelW();
    int panelH();
}
