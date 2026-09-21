#include "led.h"
#include "board.h"
#include <string.h>

/* Tiny WS2812 bitbang on ESP32-S3 @ 240 MHz.  T0H~0.4us T1H~0.8us. */
static uint8_t pix[WS2812_NUM_LEDS * 3];
static uint32_t blinkUntil = 0;
static uint8_t br = 0, bg = 0, bb = 0;
static bool blinking = false;

static void ws_send() {
    uint8_t pin = WS2812_DATA_PIN;
    pinMode(pin, OUTPUT);
    noInterrupts();
    for (int i = 0; i < WS2812_NUM_LEDS * 3; i++) {
        uint8_t v = pix[i];
        for (int b = 7; b >= 0; b--) {
            if (v & (1 << b)) {
                digitalWrite(pin, HIGH);
                __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;"
                                     "nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;"
                                     "nop;nop;nop;nop;nop;nop;nop;nop;");
                digitalWrite(pin, LOW);
                __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;"
                                     "nop;nop;nop;nop;");
            } else {
                digitalWrite(pin, HIGH);
                __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;"
                                     "nop;nop;");
                digitalWrite(pin, LOW);
                __asm__ __volatile__("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;"
                                     "nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
            }
        }
    }
    interrupts();
    delayMicroseconds(60);
}

void Led::init() {
    memset(pix, 0, sizeof(pix));
    pinMode(WS2812_DATA_PIN, OUTPUT);
    digitalWrite(WS2812_DATA_PIN, LOW);
    delay(1);
    show();
}
void Led::set(uint8_t i, uint8_t r, uint8_t g, uint8_t b) {
    if (i >= WS2812_NUM_LEDS) return;
    pix[i * 3 + 0] = g; pix[i * 3 + 1] = r; pix[i * 3 + 2] = b;
}
void Led::fill(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < WS2812_NUM_LEDS; i++) set((uint8_t)i, r, g, b);
}
void Led::show() { ws_send(); }
void Led::off() { fill(0, 0, 0); show(); blinking = false; }
void Led::blink(uint8_t r, uint8_t g, uint8_t b, uint32_t ms) {
    br = r; bg = g; bb = b;
    fill(r, g, b); show();
    blinkUntil = millis() + ms;
    blinking = true;
}
void Led::tick(uint32_t now) {
    if (blinking && now > blinkUntil) off();
}
