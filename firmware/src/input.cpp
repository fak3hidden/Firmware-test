#include "input.h"
#include "board.h"

static constexpr int QMAX = 16;
static InputEvent q[QMAX];
static volatile uint8_t qh, qt;
static volatile int32_t encAcc = 0;
static volatile uint8_t encPrev = 0;
static uint32_t lastIdle = 0;
static bool held[InputKeyMAX];
static uint32_t pressAt[InputKeyMAX];
static bool longFired[InputKeyMAX];
static uint32_t lastRepeat[InputKeyMAX];

static void push(InputKey k, InputType t) {
    uint8_t n = (uint8_t)((qh + 1) % QMAX);
    if (n == qt) return;
    q[qh] = {k, t};
    qh = n;
    lastIdle = millis();
}

void IRAM_ATTR encIsr() {
    static const int8_t tab[16] = {
        0, -1,  1,  0,
        1,  0,  0, -1,
       -1,  0,  0,  1,
        0,  1, -1,  0
    };
    uint8_t a = (uint8_t)digitalRead(ENCODER_INA);
    uint8_t b = (uint8_t)digitalRead(ENCODER_INB);
    uint8_t st = (uint8_t)((a << 1) | b);
    uint8_t idx = (uint8_t)((encPrev << 2) | st);
    encAcc += tab[idx & 15];
    encPrev = st;
}

void Input::init() {
    pinMode(ENCODER_INA, INPUT_PULLUP);
    pinMode(ENCODER_INB, INPUT_PULLUP);
    pinMode(ENCODER_KEY, INPUT_PULLUP);
    pinMode(BOARD_USER_KEY, INPUT_PULLUP);
    encPrev = (uint8_t)((digitalRead(ENCODER_INA) << 1) | digitalRead(ENCODER_INB));
    attachInterrupt(digitalPinToInterrupt(ENCODER_INA), encIsr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_INB), encIsr, CHANGE);
    lastIdle = millis();
}

static void debounce(InputKey k, bool downNow, uint32_t now) {
    if (downNow && !held[k]) {
        held[k] = true;
        pressAt[k] = now;
        longFired[k] = false;
        push(k, InputTypePress);
    } else if (!downNow && held[k]) {
        held[k] = false;
        push(k, InputTypeRelease);
        if (!longFired[k] && now - pressAt[k] < 500)
            push(k, InputTypeShort);
    } else if (downNow && held[k] && !longFired[k] && now - pressAt[k] >= 500) {
        longFired[k] = true;
        push(k, InputTypeLong);
    } else if (downNow && longFired[k] && now - lastRepeat[k] > 120) {
        lastRepeat[k] = now;
        push(k, InputTypeRepeat);
    }
}

void Input::poll() {
    uint32_t now = millis();
    int32_t acc;
    noInterrupts();
    acc = encAcc;
    encAcc = 0;
    interrupts();
    /* 4 detents per click typically; consume in steps of 2 or 4. */
    static int32_t rest = 0;
    rest += acc;
    while (rest >= 2) { rest -= 2; push(InputKeyDown, InputTypeShort); }
    while (rest <= -2) { rest += 2; push(InputKeyUp, InputTypeShort); }

    bool ok = digitalRead(ENCODER_KEY) == LOW;
    bool back = digitalRead(BOARD_USER_KEY) == LOW;
    debounce(InputKeyOk, ok, now);
    debounce(InputKeyBack, back, now);
}

bool Input::pop(InputEvent& e) {
    if (qh == qt) return false;
    e = q[qt];
    qt = (uint8_t)((qt + 1) % QMAX);
    return true;
}
bool Input::down(InputKey k) { return held[k]; }
void Input::resetIdle() { lastIdle = millis(); }
uint32_t Input::idleMs() { return millis() - lastIdle; }
