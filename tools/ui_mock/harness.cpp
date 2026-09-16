// Compiles the REAL firmware UI headers against mock canvases and dumps the
// primitive draw calls, so render.py can produce actual previews of the UI.
#include "mock.h"

// menu first (defines its own namespace-scope helpers)
#include "../../T-Embed-CC1101/examples/factory/flipper_style.h"
#include "../../T-Embed-CC1101/examples/factory/main_menu_ui.h"

// page_startup.h declares file-static probe state; flatten statics so the
// harness can drive them, like a unit test would.
#define static
#include "../../T-Embed-CC1101/examples/factory/page_startup.h"
#undef static

int main()
{
    Canvas c;

    // --- main menu, page 1 (cursor on CC1101) ---
    g.menuCursor = 1;
    drawMenuUi(c);

    // --- main menu, page 2 (cursor on Settings -> scrollbar) ---
    g.menuCursor = 10;
    gOps.push_back({"FRAME",0,0,0,0,0,0,0,""});
    drawMenuUi(c);

    // --- startup desktop (pretend healthy V1.0 hardware) ---
    page_startup::detectedProfile = page_startup::HardwareProfile::V10;
    page_startup::allowMenuEntry = true;
    page_startup::hasAddr24 = page_startup::hasAddr55 = page_startup::hasAddr6B = true;
    gOps.push_back({"FRAME",0,0,0,0,0,0,0,""});
    page_startup::drawUi(c);

    // dump ops
    for (const Op& o : gOps) {
        printf("%s %d %d %d %d %d %d %04x %s\n", o.kind, o.a, o.b, o.c, o.d, o.e, o.f, o.color, o.text.c_str());
    }
    return 0;
}
