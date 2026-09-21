#include "scene.h"

static constexpr int MAXD = 8;
static const Scene* stack[MAXD];
static int sp = 0;

void Scenes::init() {
    sp = 0;
}

void Scenes::push(const Scene* s) {
    if (sp >= MAXD || !s) return;
    if (sp > 0 && stack[sp - 1]->exit) stack[sp - 1]->exit();
    stack[sp++] = s;
    if (s->enter) s->enter();
    gCanvas.markDirty();
}

void Scenes::pop() {
    if (sp <= 1) return;
    if (stack[sp - 1]->exit) stack[sp - 1]->exit();
    sp--;
    if (stack[sp - 1]->enter) stack[sp - 1]->enter();
    gCanvas.markDirty();
}

void Scenes::switchTo(const Scene* s) {
    if (sp > 0 && stack[sp - 1]->exit) stack[sp - 1]->exit();
    if (sp == 0) sp = 1;
    stack[sp - 1] = s;
    if (s && s->enter) s->enter();
    gCanvas.markDirty();
}

const Scene* Scenes::current() { return sp ? stack[sp - 1] : nullptr; }
int Scenes::depth() { return sp; }

void Scenes::draw(Canvas& c) {
    if (sp && stack[sp - 1]->draw) stack[sp - 1]->draw(c);
}
void Scenes::input(const InputEvent& e) {
    if (sp && stack[sp - 1]->input) stack[sp - 1]->input(e);
}
void Scenes::tick(uint32_t now) {
    if (sp && stack[sp - 1]->tick) stack[sp - 1]->tick(now);
}
