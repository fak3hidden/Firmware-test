#pragma once
#include "../canvas.h"
#include "../input.h"

struct Scene;

/* Status bar: 12 px high, drawn by most scenes. */
void statusbar_draw(Canvas& c, const char* title = nullptr);

struct MenuItem {
    const char* label;
    const TfIcon* icon;     /* 24x24, may be null */
    const Scene* scene;     /* jump target, may be null */
    void (*enter)();        /* extra callback */
};

class Menu {
public:
    const char* title = "Menu";
    const MenuItem* items = nullptr;
    int count = 0;
    int sel = 0;
    int window = 0;
    int rows = 6;           /* visible rows */
    bool show_icon = true;

    void set(const char* t, const MenuItem* it, int n);
    void draw(Canvas& c);
    bool input(const InputEvent& e); /* true if consumed; Ok handled by caller via selected() */
    int  selected() const { return sel; }
};

class VarList {
public:
    struct Item {
        const char* label;
        char value[24];
        void (*change)(int dir, Item* self);
    };
    const char* title = "";
    Item* items = nullptr;
    int count = 0;
    int sel = 0;
    void draw(Canvas& c);
    bool input(const InputEvent& e);
};
