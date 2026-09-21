#include "appvm.h"
#include "storage.h"
#include "gui/widgets.h"
#include "gui/elements.h"
#include "gui/assets_fonts.h"
#include "drivers/ir.h"
#include "drivers/cc1101.h"
#include "led.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

static constexpr int MAX_APPS = 16;
static char ids[MAX_APPS][24];
static char names[MAX_APPS][24];
static int nApps = 0;
static bool gRun = false;
static String gJson;
static String gTitle, gBody;
static int gMenuSel = 0;
static char gItems[8][24];
static int gNItems = 0;
static enum { S_MENU, S_TEXT, S_DONE } gSt;

static String jstr(const String& json, const char* key) {
    String pat = String("\"") + key + "\"";
    int k = json.indexOf(pat);
    if (k < 0) return String();
    int c = json.indexOf(':', k + pat.length());
    if (c < 0) return String();
    int i = c + 1;
    while (i < (int)json.length() && (json[i] == ' ' || json[i] == '\n')) i++;
    if (i < (int)json.length() && json[i] == '"') {
        i++;
        int e = i;
        while (e < (int)json.length() && json[e] != '"') e++;
        return json.substring(i, e);
    }
    int e = i;
    while (e < (int)json.length() && json[e] != ',' && json[e] != '}' && json[e] != ']') e++;
    return json.substring(i, e);
}

void AppVM::init() {
    nApps = 0;
    char list[MAX_APPS][32];
    int n = Storage::list("/ext/apps", list, MAX_APPS, false);
    for (int i = 0; i < n && nApps < MAX_APPS; i++) {
        if (!strstr(list[i], ".tapp")) continue;
        strncpy(ids[nApps], list[i], 23); ids[nApps][23] = 0;
        String path = String("/ext/apps/") + list[i];
        String js = Storage::readText(path.c_str());
        String nm = jstr(js, "name");
        if (nm.length() == 0) nm = list[i];
        strncpy(names[nApps], nm.c_str(), 23); names[nApps][23] = 0;
        nApps++;
    }
}

int AppVM::count() { return nApps; }
bool AppVM::nameAt(int i, char* out, size_t n) {
    if (i < 0 || i >= nApps) return false;
    strncpy(out, names[i], n - 1); out[n - 1] = 0;
    return true;
}

String AppVM::listJson() {
    String s = "[";
    for (int i = 0; i < nApps; i++) {
        if (i) s += ",";
        s += "{\"id\":\""; s += ids[i]; s += "\",\"name\":\""; s += names[i]; s += "\"}";
    }
    s += "]";
    return s;
}

bool AppVM::install(const uint8_t* data, size_t n) {
    if (!data || n < 8 || n > 16000) return false;
    String js; js.reserve(n + 1);
    for (size_t i = 0; i < n; i++) js += (char)data[i];
    String id = jstr(js, "id");
    if (id.length() == 0) id = jstr(js, "name");
    if (id.length() == 0) id = "app";
    /* sanitize */
    String fn;
    for (size_t i = 0; i < id.length(); i++) {
        char c = id[i];
        if (isalnum(c) || c == '-' || c == '_') fn += c;
    }
    if (fn.length() == 0) fn = "app";
    fn = "/ext/apps/" + fn + ".tapp";
    bool ok = Storage::writeFile(fn.c_str(), data, n) == n;
    init();
    return ok;
}

bool AppVM::uninstall(const char* id) {
    String p = String("/ext/apps/") + id;
    bool ok = Storage::remove(p.c_str());
    init();
    return ok;
}

static void loadMenuFrom(const String& js) {
    gTitle = jstr(js, "name");
    gNItems = 0;
    gMenuSel = 0;
    int items = js.indexOf("\"items\"");
    if (items < 0) {
        gBody = jstr(js, "text");
        if (gBody.length() == 0) gBody = jstr(js, "description");
        gSt = S_TEXT;
        return;
    }
    int i = js.indexOf('[', items);
    if (i < 0) { gSt = S_TEXT; return; }
    i++;
    while (gNItems < 8 && i < (int)js.length()) {
        int q = js.indexOf('"', i);
        if (q < 0) break;
        int q2 = js.indexOf('"', q + 1);
        if (q2 < 0) break;
        String lab = js.substring(q + 1, q2);
        strncpy(gItems[gNItems], lab.c_str(), 23); gItems[gNItems][23] = 0;
        gNItems++;
        i = q2 + 1;
        int br = js.indexOf(']', i);
        int cm = js.indexOf(',', i);
        if (br >= 0 && (cm < 0 || br < cm)) break;
        if (cm < 0) break;
        i = cm + 1;
    }
    gSt = S_MENU;
}

bool AppVM::run(int i) {
    if (i < 0 || i >= nApps) return false;
    String path = String("/ext/apps/") + ids[i];
    gJson = Storage::readText(path.c_str());
    if (gJson.length() < 2) return false;
    loadMenuFrom(gJson);
    gRun = true;
    Scenes::push(&scene_app_run);
    return true;
}
bool AppVM::runId(const char* id) {
    for (int i = 0; i < nApps; i++) if (!strcmp(ids[i], id)) return run(i);
    return false;
}
bool AppVM::running() { return gRun; }
void AppVM::stop() { gRun = false; }

void AppVM::draw(Canvas& c) {
    c.clear(0);
    app_header(c, gTitle.c_str());
    if (gSt == S_MENU) {
        for (int i = 0; i < gNItems; i++) {
            int y = 14 + i * 8;
            if (i == gMenuSel) {
                c.rbox(2, y - 1, 124, 8, 2);
                c.setColor(0);
                c.text(6, y, gItems[i], &tf_primary);
                c.setColor(1);
            } else {
                c.text(6, y, gItems[i], &tf_primary);
            }
        }
    } else {
        c.text(4, 20, gBody.c_str(), &tf_primary);
        c.text(4, 54, "OK to exit", &tf_secondary);
    }
}

void AppVM::input(const InputEvent& e) {
    if (e.type != InputTypeShort) {
        if (e.key == InputKeyBack && (e.type == InputTypeShort || e.type == InputTypeLong)) {
            gRun = false; Scenes::pop();
        }
        return;
    }
    if (e.key == InputKeyBack) { gRun = false; Scenes::pop(); return; }
    if (gSt == S_MENU) {
        if (e.key == InputKeyUp && gNItems) { gMenuSel = (gMenuSel + gNItems - 1) % gNItems; gCanvas.markDirty(); }
        if (e.key == InputKeyDown && gNItems) { gMenuSel = (gMenuSel + 1) % gNItems; gCanvas.markDirty(); }
        if (e.key == InputKeyOk) {
            /* interpret item: if JSON has ir_send / led / popup */
            String proto = jstr(gJson, "protocol");
            String action = jstr(gJson, "action");
            if (action == "ir_send" || proto == "NEC") {
                uint32_t d = (uint32_t)strtoul(jstr(gJson, "data").c_str(), nullptr, 0);
                IR::sendNEC(d);
                popup_show("IR", "Sent", 800);
            } else if (action == "led") {
                Led::blink(255, 80, 0, 400);
            } else {
                gBody = gItems[gMenuSel];
                gSt = S_TEXT;
                gCanvas.markDirty();
            }
        }
    } else if (e.key == InputKeyOk) {
        gRun = false; Scenes::pop();
    }
}
void AppVM::tick(uint32_t) {}

static void app_enter() {}
static void app_exit() { gRun = false; }
static void app_draw(Canvas& c) { AppVM::draw(c); }
static void app_input(const InputEvent& e) { AppVM::input(e); }
const Scene scene_app_run = { "App", app_enter, app_exit, app_draw, app_input, nullptr };
