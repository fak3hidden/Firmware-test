// Mock Arduino/TFT environment to compile the real, unmodified UI headers
// (flipper_style.h, main_menu_ui.h, page_startup.h) on a desktop compiler.
//
// Build (from repo root, after `update.bat`/`git clone` produced T-Embed-CC1101):
//   g++ -std=c++14 -o harness harness.cpp
//   ./harness > ops.txt
//   python3 render.py          # pip install pillow
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>

// ---------------------------------------------------------------- String lite
class String {
public:
    std::string s;
    String() = default;
    String(const char* p) : s(p ? p : "") {}
    String(const std::string& p) : s(p) {}
    String(long v) : s(std::to_string(v)) {}
    String(int v) : s(std::to_string(v)) {}
    String(unsigned v) : s(std::to_string(v)) {}
    String(unsigned long v) : s(std::to_string(v)) {}

    unsigned int length() const { return s.size(); }
    bool isEmpty() const { return s.empty(); }
    char charAt(unsigned i) const { return i < s.size() ? s[i] : '\0'; }
    const char* c_str() const { return s.c_str(); }
    bool startsWith(const char* p) const { return s.rfind(p, 0) == 0; }
    long toInt() const { try { return std::stol(s); } catch (...) { return 0; } }
    String substring(unsigned a) const { return a < s.size() ? String(s.substr(a)) : String(""); }
    String substring(unsigned a, unsigned b) const { return a < s.size() ? String(s.substr(a, b - a)) : String(""); }
    void trim() {
        const auto f = s.find_first_not_of(" \t\r\n");
        const auto l = s.find_last_not_of(" \t\r\n");
        s = (f == std::string::npos) ? "" : s.substr(f, l - f + 1);
    }
    void replace(const char* from, const char* to) {
        const std::string f(from), t(to);
        size_t pos = 0;
        while ((pos = s.find(f, pos)) != std::string::npos) { s.replace(pos, f.size(), t); pos += t.size(); }
    }
    String operator+(const String& o) const { return String(s + o.s); }
    String operator+(const char* o) const { return String(s + (o ? o : "")); }
};

inline bool isDigit(char c) { return c >= '0' && c <= '9'; }

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
template <typename T> T min(T a, T b) { return a < b ? a : b; }

// more Arduino / firmware-environment stubs used by page_startup.h
constexpr uint16_t TFT_YELLOW = 0xFFE0;
constexpr uint16_t kPassBg = 0xE7DC;
constexpr uint16_t kFailBg = 0xFEBA;
constexpr uint8_t OUTPUT = 1;
constexpr uint8_t INPUT_PULLUP = 2;
constexpr uint8_t LOW = 0;
constexpr uint8_t HIGH = 1;
constexpr uint8_t BOARD_PN532_RF_REST = 41;
constexpr uint8_t BOARD_PN532_IRQ = 40;
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline void delay(uint32_t) {}
inline uint32_t millis() { return 0; }
struct WireStub {
    void setClock(uint32_t) {}
    void setTimeOut(uint32_t) {}
    void beginTransmission(uint8_t) {}
    int endTransmission() { return 0; }
};
static WireStub Wire;
inline bool i2cDevicePresent(uint8_t) { return false; }
struct SerialStub {
    template <typename... A> void printf(const char*, A...) {}
    void println() {}
    void println(const char*) {}
};
static SerialStub Serial;
#define F(x) x
inline void board_prepare_display() {}
inline void board_spi_deselect_all() {}

// colors / datums
constexpr uint16_t TFT_WHITE = 0xFFFF;
constexpr uint16_t TFT_BLACK = 0x0000;
constexpr uint16_t TFT_GREEN = 0x07E0;
constexpr uint16_t TFT_RED   = 0xF800;
constexpr uint16_t TFT_LIGHTGREY = 0xD69A;
constexpr uint8_t TL_DATUM = 0;
constexpr uint8_t TR_DATUM = 2;
constexpr uint8_t MC_DATUM = 4;

constexpr uint16_t kUiMuted = 0x7BEF;
constexpr int16_t kHeaderH = 24;
constexpr int16_t kFooterH = 18;
constexpr int16_t kMargin  = 8;

// build-info globals (normally generated)
const char kFactorySoftwareVersion[] = "v1.0.0";
const char kFactoryGitHash[] = "4eb31f8";
const char kFactoryBuildStamp[] = "2026-09-16 12:00";

// ------------------------------------------------------------- drawing ops
struct Op {
    const char* kind;
    int a, b, c, d, e, f;
    uint16_t color;
    std::string text;
};
static std::vector<Op> gOps;
static int gDatum = 0;
static uint16_t gFg = TFT_BLACK, gBg = TFT_WHITE;

class Canvas {
public:
    int16_t width() { return 320; }
    int16_t height() { return 170; }
    void fillScreen(uint32_t c) { gOps.push_back({"fillScreen", 0,0,0,0,0,0, (uint16_t)c, ""}); }
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) { gOps.push_back({"fillRect", x,y,w,h,0,0,(uint16_t)c,""}); }
    void drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) { gOps.push_back({"drawRect", x,y,w,h,0,0,(uint16_t)c,""}); }
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t c) { gOps.push_back({"fillRoundRect", x,y,w,h,r,0,(uint16_t)c,""}); }
    void drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t c) { gOps.push_back({"drawRoundRect", x,y,w,h,r,0,(uint16_t)c,""}); }
    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t c) { gOps.push_back({"drawFastHLine", x,y,w,0,0,0,(uint16_t)c,""}); }
    void drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t c) { gOps.push_back({"drawFastVLine", x,y,h,0,0,0,(uint16_t)c,""}); }
    void fillCircle(int32_t x, int32_t y, int32_t r, uint32_t c) { gOps.push_back({"fillCircle", x,y,r,0,0,0,(uint16_t)c,""}); }
    void drawCircle(int32_t x, int32_t y, int32_t r, uint32_t c) { gOps.push_back({"drawCircle", x,y,r,0,0,0,(uint16_t)c,""}); }
    void fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t c) { gOps.push_back({"fillEllipse", x,y,rx,ry,0,0,c,""}); }
    void fillTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t c) { gOps.push_back({"fillTriangle", x0,y0,x1,y1,x2,y2,(uint16_t)c,""}); }
    void drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t c) { gOps.push_back({"drawLine", x0,y0,x1,y1,0,0,(uint16_t)c,""}); }
    void drawPixel(int32_t x, int32_t y, uint32_t c) { gOps.push_back({"drawPixel", x,y,0,0,0,0,(uint16_t)c,""}); }

    void setTextDatum(uint8_t d) { gDatum = d; }
    void setTextColor(uint16_t fg) { gFg = fg; gBg = fg; }
    void setTextColor(uint16_t fg, uint16_t bg) { gFg = fg; gBg = bg; }

    int16_t drawString(const String& t, int32_t x, int32_t y, uint8_t f = 1) { return drawStr(t.s, x, y, f, "L"); }
    int16_t drawString(const char* t, int32_t x, int32_t y, uint8_t f = 1) { return drawStr(t ? t : "", x, y, f, "L"); }
    int16_t drawCentreString(const String& t, int32_t x, int32_t y, uint8_t f = 1) { return drawStr(t.s, x, y, f, "C"); }
    int16_t drawCentreString(const char* t, int32_t x, int32_t y, uint8_t f = 1) { return drawStr(t ? t : "", x, y, f, "C"); }
    int16_t drawRightString(const String& t, int32_t x, int32_t y, uint8_t f = 1) { return drawStr(t.s, x, y, f, "R"); }
    int16_t drawRightString(const char* t, int32_t x, int32_t y, uint8_t f = 1) { return drawStr(t ? t : "", x, y, f, "R"); }
private:
    int16_t drawStr(const std::string& t, int32_t x, int32_t y, uint8_t f, const char* align) {
        Op op; op.kind = "text"; op.a = x; op.b = y; op.c = f; op.d = gDatum;
        op.e = align[0]; op.f = 0; op.color = gFg;
        char bgbuf[8]; snprintf(bgbuf, sizeof(bgbuf), "%04x", gBg);
        op.text = std::string(bgbuf) + "|" + t;
        gOps.push_back(op);
        return (int16_t)t.size() * 6;
    }
};

// sprite + tft fakes for page_startup.h
class FakeTft : public Canvas {
};
class TFT_eSprite : public Canvas {
public:
    explicit TFT_eSprite(FakeTft*) {}
    void setColorDepth(int) {}
    void* createSprite(int16_t, int16_t) { return (void*)1; }
    void deleteSprite() {}
    void pushSprite(int16_t, int16_t) {}
};
static FakeTft tft;

// ------------------------------------------------------ factory.cpp context
struct GStub { int8_t menuCursor = 0; volatile int32_t encRaw = 0; } g;
inline bool takeUserButton() { return false; }
inline bool takeEncoderButton() { return false; }
inline int32_t takeEncoderDelta(int32_t&) { return 0; }

constexpr uint8_t kPageCount = 11;
struct PageDescriptor { const char* label; };
static const PageDescriptor kPages[kPageCount] = {
    {"Battery / PMU"}, {"CC1101 Radio"}, {"IR TX/RX"}, {"MIC & Speaker"},
    {"PN532 NFC"}, {"nRF24 Tx/Rx"}, {"SD Card"}, {"WiFi"}, {"TFT Display"},
    {"WS2812 LEDs"}, {"Settings"},
};

namespace page_battery {
String menuPreviewLine1() { return "SOC: 78%"; }
String menuPreviewLine2() { return "CHG: not charging"; }
uint16_t menuPreviewLine1Color() { return TFT_GREEN; }
uint16_t menuPreviewLine2Color() { return TFT_LIGHTGREY; }
}

String currentRotationLabel() { return "Landscape"; }
String autoSleepPresetLabel() { return "1m"; }
String autoDimTimeoutLabel() { return "40s"; }
