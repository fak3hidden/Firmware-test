#pragma once
#include <stdint.h>

enum InputKey : uint8_t {
    InputKeyUp = 0,
    InputKeyDown,
    InputKeyLeft,
    InputKeyRight,
    InputKeyOk,
    InputKeyBack,
    InputKeyMAX
};

enum InputType : uint8_t {
    InputTypePress = 0,
    InputTypeRelease,
    InputTypeShort,
    InputTypeLong,
    InputTypeRepeat
};

struct InputEvent {
    InputKey key;
    InputType type;
};

namespace Input {
    void init();
    void poll();
    bool pop(InputEvent& e);
    bool down(InputKey k);
    void resetIdle();
    uint32_t idleMs();
}
