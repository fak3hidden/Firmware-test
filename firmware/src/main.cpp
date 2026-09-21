#include <Arduino.h>
#include "version.h"
#include "board.h"
#include "display.h"
#include "input.h"
#include "canvas.h"
#include "scene.h"
#include "storage.h"
#include "protocol.h"
#include "led.h"
#include "appvm.h"
#include "drivers/cc1101.h"
#include "drivers/pn532.h"
#include "drivers/ir.h"
#include "drivers/nrf24.h"

void setup() {
    Board::init();
    Display::init();
    Input::init();
    Led::init();
    Storage::init();
    Protocol::init();

    Serial.printf("\n%s %s  %s\n", FINOS_NAME, FINOS_VERSION, FINOS_BOARD_NAME);

    CC1101::init();
    NFC::init();
    IR::init();
    if (Board::isPlus()) NRF24::init();
    AppVM::init();

    Scenes::init();
    Scenes::push(&scene_desktop);

    Led::blink(255, 90, 0, 350);
    gCanvas.markDirty();
}

void loop() {
    Input::poll();
    InputEvent e;
    while (Input::pop(e)) {
        Scenes::input(e);
        gCanvas.markDirty();
    }
    uint32_t now = millis();
    Scenes::tick(now);
    Led::tick(now);
    Protocol::poll();

    if (gCanvas.dirty) {
        Scenes::draw(gCanvas);
        Display::present(gCanvas);
    }
    delay(4);
}
