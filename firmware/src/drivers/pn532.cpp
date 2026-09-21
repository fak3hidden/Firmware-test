#include "pn532.h"
#include "../board.h"
#include <string.h>

static bool gOk = false;
static constexpr uint8_t ADDR = BOARD_PN532_ADDR;

static bool writeRaw(const uint8_t* d, size_t n) {
    Wire.beginTransmission(ADDR);
    for (size_t i = 0; i < n; i++) Wire.write(d[i]);
    return Wire.endTransmission() == 0;
}

static bool readRaw(uint8_t* d, size_t n) {
    size_t g = Wire.requestFrom(ADDR, (uint8_t)n);
    for (size_t i = 0; i < g; i++) d[i] = (uint8_t)Wire.read();
    return g == n;
}

static bool sendFrame(const uint8_t* cmd, size_t n) {
    /* preamble 00 00 FF, LEN, LCS, TFI=D4, data, DCS, postamble 00 */
    uint8_t buf[64];
    size_t i = 0;
    buf[i++] = 0x00; buf[i++] = 0x00; buf[i++] = 0xFF;
    uint8_t len = (uint8_t)(n + 1);
    buf[i++] = len;
    buf[i++] = (uint8_t)(~len + 1);
    buf[i++] = 0xD4;
    uint8_t sum = 0xD4;
    for (size_t k = 0; k < n; k++) { buf[i++] = cmd[k]; sum = (uint8_t)(sum + cmd[k]); }
    buf[i++] = (uint8_t)(~sum + 1);
    buf[i++] = 0x00;
    return writeRaw(buf, i);
}

static int recvFrame(uint8_t* data, size_t maxn, uint32_t timeout = 200) {
    uint32_t t0 = millis();
    uint8_t tmp[64];
    while (millis() - t0 < timeout) {
        if (!readRaw(tmp, 7)) { delay(2); continue; }
        /* look for 00 00 FF */
        int s = -1;
        for (int i = 0; i < 5; i++) {
            if (tmp[i] == 0x00 && tmp[i + 1] == 0x00 && tmp[i + 2] == 0xFF) { s = i; break; }
        }
        if (s < 0) { delay(2); continue; }
        uint8_t len = tmp[s + 3];
        if (len == 0 || len > 40) { delay(2); continue; }
        /* need rest of frame */
        uint8_t rest[48];
        if (!readRaw(rest, len + 2)) continue;
        /* data starts after TFI */
        uint8_t tfi = 0;
        /* simpler: reread whole thing */
        (void)rest; (void)tfi;
        /* Use a fresh full read */
        delay(1);
        uint8_t full[48];
        if (!readRaw(full, len + 8)) continue;
        /* find 00 00 FF LEN LCS D5 ... */
        for (int i = 0; i < 8; i++) {
            if (full[i] == 0x00 && full[i + 1] == 0x00 && full[i + 2] == 0xFF) {
                uint8_t L = full[i + 3];
                if (L == 0 || L + i + 6 > 48) break;
                uint8_t tfi2 = full[i + 5];
                if (tfi2 != 0xD5) break;
                size_t payload = L - 1;
                if (payload > maxn) payload = maxn;
                memcpy(data, &full[i + 6], payload);
                return (int)payload;
            }
        }
    }
    return -1;
}

/* More robust PN532 I2C: ready byte 0x01 then frame. */
static bool waitReady(uint32_t ms) {
    uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        Wire.requestFrom(ADDR, (uint8_t)1);
        if (Wire.available()) {
            uint8_t r = (uint8_t)Wire.read();
            if (r & 0x01) return true;
        }
        delay(2);
    }
    return false;
}

static int transceive(const uint8_t* cmd, size_t n, uint8_t* resp, size_t maxr, uint32_t to = 250) {
    if (!sendFrame(cmd, n)) return -1;
    delay(2);
    if (!waitReady(to)) return -1;
    uint8_t buf[64];
    size_t want = 7;
    if (Wire.requestFrom(ADDR, (uint8_t)64) == 0) return -1;
    int got = 0;
    while (Wire.available() && got < 64) buf[got++] = (uint8_t)Wire.read();
    /* skip ready byte */
    int i = 0;
    if (got && buf[0] == 0x01) i = 1;
    while (i + 5 < got) {
        if (buf[i] == 0x00 && buf[i + 1] == 0x00 && buf[i + 2] == 0xFF) {
            uint8_t L = buf[i + 3];
            if (L == 0x00 && buf[i + 4] == 0xFF && buf[i + 5] == 0x00) {
                /* ACK frame, wait data */
                if (!waitReady(to)) return -1;
                got = 0;
                Wire.requestFrom(ADDR, (uint8_t)64);
                while (Wire.available() && got < 64) buf[got++] = (uint8_t)Wire.read();
                i = (got && buf[0] == 0x01) ? 1 : 0;
                continue;
            }
            if (L == 0 || i + 6 + L > got) break;
            if (buf[i + 5] != 0xD5) break;
            size_t payload = L - 1;
            if (payload > maxr) payload = maxr;
            memcpy(resp, &buf[i + 6], payload);
            return (int)payload;
        }
        i++;
    }
    return -1;
}

bool NFC::init() {
    pinMode(BOARD_PN532_RF_REST, OUTPUT);
    digitalWrite(BOARD_PN532_RF_REST, HIGH);
    delay(2);
    digitalWrite(BOARD_PN532_RF_REST, LOW);
    delay(2);
    digitalWrite(BOARD_PN532_RF_REST, HIGH);
    delay(10);
    /* wakeup preamble */
    uint8_t wake[] = {0x00, 0x00, 0xFF, 0x05, 0xFB, 0xD4, 0x14, 0x01, 0x00, 0x00, 0xE9, 0x00};
    writeRaw(wake, sizeof(wake));
    delay(20);
    uint8_t sam[] = {0x14, 0x01, 0x00, 0x00}; /* SAMConfiguration normal */
    uint8_t resp[16];
    int n = transceive(sam, 4, resp, sizeof(resp), 200);
    gOk = n >= 1 && resp[0] == 0x15;
    if (!gOk) {
        /* GetFirmwareVersion */
        uint8_t gf[] = {0x02};
        n = transceive(gf, 1, resp, sizeof(resp), 200);
        gOk = n >= 2 && resp[0] == 0x03;
    }
    return gOk;
}
bool NFC::present() { return gOk; }

uint8_t NFC::pollA(uint8_t* uid, uint8_t maxuid, uint16_t* atqa, uint8_t* sak) {
    uint8_t cmd[] = {0x4A, 0x01, 0x00}; /* InListPassiveTarget, 1, 106kbps type A */
    uint8_t r[32];
    int n = transceive(cmd, 3, r, sizeof(r), 150);
    if (n < 6 || r[0] != 0x4B || r[1] < 1) return 0;
    /* r: 4B NbTg Tg SENS_RES(2) SEL_RES NFCIDLen NFCID */
    if (atqa) *atqa = (uint16_t)(r[3] << 8 | r[4]);
    if (sak) *sak = r[5];
    uint8_t len = r[6];
    if (len > maxuid) len = maxuid;
    if (7 + len > n) return 0;
    memcpy(uid, &r[7], len);
    return len;
}

bool NFC::readNdef(char* out, size_t maxn) {
    if (!out || !maxn) return false;
    out[0] = 0;
    return false; /* keep simple: UID only in v0.1 */
}
void NFC::sleep() {}
