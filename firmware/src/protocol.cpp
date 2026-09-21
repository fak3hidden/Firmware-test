#include "protocol.h"
#include "version.h"
#include "board.h"
#include "storage.h"
#include "canvas.h"
#include "led.h"
#include "appvm.h"
#include "ble.h"
#include <string.h>

/* Text CLI on USB CDC, plus a framed binary mode.
 * Frame: 'T' 'F' 0x01 cmd:u8 len:u16le payload crc8
 * If a line doesn't start with TF\x01, treat as CLI.
 */

static bool gHost = false;
static uint32_t lastByte = 0;
static uint8_t acc[2048];
static int accn = 0;

static uint8_t crc8(const uint8_t* p, int n) {
    uint8_t c = 0x00;
    for (int i = 0; i < n; i++) {
        c ^= p[i];
        for (int b = 0; b < 8; b++) c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
    }
    return c;
}

static uint8_t bleIn[2048];
static int bleInN = 0;

static void sendFrame(uint8_t cmd, const uint8_t* p, uint16_t n) {
    static uint8_t hdr[7 + 1800];
    if (n >= 1800) { n = 0; p = nullptr; }
    hdr[0] = 'T'; hdr[1] = 'F'; hdr[2] = 0x01; hdr[3] = cmd;
    hdr[4] = (uint8_t)(n & 0xFF);
    hdr[5] = (uint8_t)(n >> 8);
    if (n && p) memcpy(hdr + 6, p, n);
    uint8_t cr = cmd;
    cr = crc8(&cr, 1);
    uint8_t ln[2] = {hdr[4], hdr[5]};
    cr ^= crc8(ln, 2);
    if (n && p) cr ^= crc8(p, n);
    hdr[6 + n] = cr;
    size_t tot = 7 + n;
    Serial.write(hdr, tot);
    Ble::send(hdr, tot);
}

void Protocol::fromBle(const uint8_t* data, size_t n) {
    if (!data || !n) return;
    if (bleInN + (int)n > (int)sizeof(bleIn)) bleInN = 0;
    size_t k = n;
    if (bleInN + (int)k > (int)sizeof(bleIn)) k = sizeof(bleIn) - bleInN;
    memcpy(bleIn + bleInN, data, k);
    bleInN += (int)k;
}

static void sendStr(uint8_t cmd, const char* s) {
    sendFrame(cmd, (const uint8_t*)s, (uint16_t)strlen(s));
}

static void handleCmd(uint8_t cmd, const uint8_t* p, uint16_t n) {
    switch (cmd) {
        case 0x01: { /* PING */
            char buf[96];
            snprintf(buf, sizeof(buf), "{\"fw\":\"%s\",\"board\":\"%s\",\"plus\":%s}",
                     FINOS_VERSION, FINOS_BOARD_ID, Board::isPlus() ? "true" : "false");
            sendStr(0x81, buf);
            break;
        }
        case 0x02: { /* INFO */
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "{\"name\":\"%s\",\"version\":\"%s\",\"board\":\"%s\",\"cc1101\":%s,"
                     "\"nfc\":%s,\"nrf24\":%s,\"sd\":%s,\"batt\":%u,\"storage\":%llu}",
                     FINOS_NAME, FINOS_VERSION, FINOS_BOARD_NAME,
                     Board::cc1101Present() ? "true" : "false",
                     Board::nfcPresent() ? "true" : "false",
                     Board::nrfPresent() ? "true" : "false",
                     Board::sdPresent() ? "true" : "false",
                     Board::batteryPercent(),
                     (unsigned long long)Storage::totalBytes());
            sendStr(0x82, buf);
            break;
        }
        case 0x03: { /* LS path */
            char path[96]; int m = n < 95 ? n : 95;
            memcpy(path, p, m); path[m] = 0;
            char names[32][32];
            int cnt = Storage::list(path, names, 32, false);
            String out;
            for (int i = 0; i < cnt; i++) { if (i) out += "\n"; out += names[i]; }
            sendStr(0x83, out.c_str());
            break;
        }
        case 0x04: { /* GET path */
            char path[96]; int m = n < 95 ? n : 95;
            memcpy(path, p, m); path[m] = 0;
            uint8_t buf[1024];
            size_t r = Storage::readFile(path, buf, sizeof(buf));
            sendFrame(0x84, buf, (uint16_t)r);
            break;
        }
        case 0x05: { /* PUT path\0 data */
            const uint8_t* z = (const uint8_t*)memchr(p, 0, n);
            if (!z) { sendStr(0x85, "ERR"); break; }
            const char* path = (const char*)p;
            const uint8_t* data = z + 1;
            size_t dn = n - (size_t)(data - p);
            size_t w = Storage::writeFile(path, data, dn);
            char ok[16]; snprintf(ok, sizeof(ok), "%u", (unsigned)w);
            sendStr(0x85, ok);
            break;
        }
        case 0x06: { /* RM */
            char path[96]; int m = n < 95 ? n : 95;
            memcpy(path, p, m); path[m] = 0;
            sendStr(0x86, Storage::remove(path) ? "OK" : "ERR");
            break;
        }
        case 0x07: { /* MKDIR */
            char path[96]; int m = n < 95 ? n : 95;
            memcpy(path, p, m); path[m] = 0;
            sendStr(0x87, Storage::mkdirp(path) ? "OK" : "ERR");
            break;
        }
        case 0x08: { /* INSTALL .tapp bytes */
            bool ok = AppVM::install(p, n);
            sendStr(0x88, ok ? "OK" : "ERR");
            break;
        }
        case 0x0A: ESP.restart(); break;
        case 0x0B: { /* SCREENSHOT 1024 bytes */
            sendFrame(0x8B, gCanvas.buf, CANVAS_BUF);
            break;
        }
        case 0x0C: { /* LED r g b */
            if (n >= 3) { Led::fill(p[0], p[1], p[2]); Led::show(); }
            sendStr(0x8C, "OK");
            break;
        }
        case 0x0D: { /* DFU: hold boot on reset isn't possible; just restart into download if GPIO0 low next. */
            sendStr(0x8D, "OK");
            delay(50);
            ESP.restart();
            break;
        }
        case 0x0E: {
            sendStr(0x8E, AppVM::listJson().c_str());
            break;
        }
        default:
            sendStr(0xFF, "UNKNOWN");
            break;
    }
}

static void handleLine(char* line) {
    while (*line == ' ') line++;
    if (!*line) return;
    if (!strcmp(line, "help") || !strcmp(line, "?")) {
        Serial.println(F("FinOS CLI: help info ls <p> cat <p> rm <p> reboot ping"));
    } else if (!strcmp(line, "info") || !strcmp(line, "ping")) {
        Serial.printf("%s %s on %s\n", FINOS_NAME, FINOS_VERSION, FINOS_BOARD_NAME);
    } else if (!strncmp(line, "ls", 2)) {
        const char* p = line + 2; while (*p == ' ') p++;
        if (!*p) p = "/ext";
        char names[32][32];
        int n = Storage::list(p, names, 32, false);
        for (int i = 0; i < n; i++) Serial.println(names[i]);
        Serial.printf("(%d)\n", n);
    } else if (!strncmp(line, "cat ", 4)) {
        Serial.println(Storage::readText(line + 4));
    } else if (!strncmp(line, "rm ", 3)) {
        Serial.println(Storage::remove(line + 3) ? "OK" : "ERR");
    } else if (!strcmp(line, "reboot")) {
        Serial.println("bye"); delay(30); ESP.restart();
    } else {
        Serial.println(F("?"));
    }
}

void Protocol::init() {
    Serial.begin(115200);
    accn = 0;
}

void Protocol::poll() {
    if (bleInN >= 7 && bleIn[0] == 'T' && bleIn[1] == 'F' && bleIn[2] == 0x01) {
        uint16_t n = (uint16_t)(bleIn[4] | (bleIn[5] << 8));
        int need = 6 + n + 1;
        if (n <= 1800 && bleInN >= need) {
            handleCmd(bleIn[3], bleIn + 6, n);
            memmove(bleIn, bleIn + need, bleInN - need);
            bleInN -= need;
        }
    } else if (bleInN >= 3 && !(bleIn[0] == 'T' && bleIn[1] == 'F')) {
        bleInN = 0;
    }
    while (Serial.available()) {
        gHost = true;
        lastByte = millis();
        uint8_t b = (uint8_t)Serial.read();
        if (accn == 0 && b != 'T') {
            /* CLI mode: gather a line */
            static char line[160];
            static int ln = 0;
            if (b == '\r') continue;
            if (b == '\n') { line[ln] = 0; handleLine(line); ln = 0; continue; }
            if (ln < 159) line[ln++] = (char)b;
            continue;
        }
        if (accn < (int)sizeof(acc)) acc[accn++] = b;
        /* try parse frame */
        if (accn >= 7 && acc[0] == 'T' && acc[1] == 'F' && acc[2] == 0x01) {
            uint16_t n = (uint16_t)(acc[4] | (acc[5] << 8));
            int need = 6 + n + 1;
            if (n > 1800) { accn = 0; continue; }
            if (accn >= need) {
                uint8_t cmd = acc[3];
                handleCmd(cmd, acc + 6, n);
                accn = 0;
            }
        } else if (accn >= 3 && !(acc[0] == 'T' && acc[1] == 'F')) {
            accn = 0;
        }
    }
    if (gHost && millis() - lastByte > 4000) gHost = false;
}

bool Protocol::hostConnected() { return gHost || Serial; }
