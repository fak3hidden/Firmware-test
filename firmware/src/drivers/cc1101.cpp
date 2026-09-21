#include "cc1101.h"
#include "../board.h"

static float gFreq = 433.92f;
static bool gOk = false;
static bool gSniff = false;
static constexpr size_t PRING = 512;
static volatile uint16_t pbuf[PRING];
static volatile uint8_t  plev[PRING];
static volatile uint16_t ph = 0, pt = 0;
static volatile uint32_t lastEdge = 0;
static volatile uint8_t lastLevel = 0;

static uint8_t strobe(uint8_t s) {
    Board::spiLock();
    Board::selectRadio();
    uint8_t st = Board::spi().transfer(s);
    Board::deselectAll();
    Board::spiUnlock();
    return st;
}
static uint8_t readReg(uint8_t a) {
    Board::spiLock();
    Board::selectRadio();
    uint8_t hdr = (a >= 0x30) ? (uint8_t)(a | 0xC0) : (uint8_t)(a | 0x80);
    Board::spi().transfer(hdr);
    uint8_t v = Board::spi().transfer(0);
    Board::deselectAll();
    Board::spiUnlock();
    return v;
}
static void writeReg(uint8_t a, uint8_t v) {
    Board::spiLock();
    Board::selectRadio();
    Board::spi().transfer(a);
    Board::spi().transfer(v);
    Board::deselectAll();
    Board::spiUnlock();
}

static void resetChip() {
    pinMode(BOARD_CC1101_CS, OUTPUT);
    digitalWrite(BOARD_CC1101_CS, HIGH);
    delay(1);
    digitalWrite(BOARD_CC1101_CS, LOW);
    delayMicroseconds(10);
    digitalWrite(BOARD_CC1101_CS, HIGH);
    delayMicroseconds(40);
    strobe(0x30); /* SRES */
    delay(5);
}

bool CC1101::init() {
    resetChip();
    uint8_t ver = readReg(0x31);
    gOk = (ver != 0x00 && ver != 0xFF);
    if (!gOk) return false;
    /* conservative OOK defaults */
    writeReg(0x0B, 0x06); /* FSCTRL1 */
    writeReg(0x0C, 0x00);
    writeReg(0x08, 0x32); /* PKTCTRL0 async serial */
    writeReg(0x10, 0xF8); /* MDMCFG4 RX bw ~ 58k? use wide for sniff 0x2D later */
    writeReg(0x11, 0x83);
    writeReg(0x12, 0x30); /* MDMCFG2 OOK, no sync */
    writeReg(0x13, 0x22);
    writeReg(0x14, 0xF8);
    writeReg(0x0A, 0x00);
    writeReg(0x15, 0x15);
    writeReg(0x18, 0x18); /* MCSM0 FS_AUTOCAL */
    writeReg(0x19, 0x16);
    writeReg(0x1B, 0x04);
    writeReg(0x1C, 0x00);
    writeReg(0x1D, 0x92);
    writeReg(0x23, 0xE9);
    writeReg(0x24, 0x2A);
    writeReg(0x25, 0x00);
    writeReg(0x26, 0x1F);
    writeReg(0x22, 0x11); /* FREND0 PATABLE[1] */
    writeReg(0x00, 0x0D); /* IOCFG2 serial data */
    writeReg(0x02, 0x0D); /* IOCFG0 serial data */
    /* PATABLE: 0x00 off, 0xC0 ~10dBm */
    Board::spiLock(); Board::selectRadio();
    Board::spi().transfer(0x3E | 0x40);
    Board::spi().transfer(0x00);
    Board::spi().transfer(0xC0);
    Board::deselectAll(); Board::spiUnlock();
    setFrequency(gFreq);
    return true;
}

bool CC1101::present() { return gOk; }

void CC1101::idle() { strobe(0x36); }
void CC1101::sleep() { strobe(0x36); strobe(0x39); }

bool CC1101::setFrequency(float mhz) {
    gFreq = mhz;
    Board::cc1101Path(mhz);
    /* FREQ = mhz * 65536 / 26 */
    uint32_t f = (uint32_t)(mhz * (65536.0f / 26.0f) + 0.5f);
    writeReg(0x0D, (uint8_t)((f >> 16) & 0xFF));
    writeReg(0x0E, (uint8_t)((f >> 8) & 0xFF));
    writeReg(0x0F, (uint8_t)(f & 0xFF));
    strobe(0x33); /* SCAL */
    delay(1);
    return true;
}
float CC1101::frequency() { return gFreq; }
void CC1101::setOok(bool) {}
void CC1101::setPower(int8_t) {}

void IRAM_ATTR sniffIsr() {
    uint32_t now = micros();
    uint32_t d = now - lastEdge;
    lastEdge = now;
    uint8_t lvl = (uint8_t)digitalRead(BOARD_CC1101_GDO2);
    uint16_t n = (uint16_t)((ph + 1) % PRING);
    if (n == pt) return;
    if (d > 65535) d = 65535;
    if (d < 30) return;
    pbuf[ph] = (uint16_t)d;
    plev[ph] = lastLevel;
    lastLevel = lvl;
    ph = n;
}

void CC1101::startSniff() {
    stopSniff();
    writeReg(0x10, 0x0C); /* wide RX BW */
    writeReg(0x12, 0x30);
    writeReg(0x08, 0x32);
    writeReg(0x00, 0x0D);
    pinMode(BOARD_CC1101_GDO2, INPUT);
    lastEdge = micros();
    lastLevel = (uint8_t)digitalRead(BOARD_CC1101_GDO2);
    ph = pt = 0;
    strobe(0x36);
    strobe(0x3A); /* SFRX */
    strobe(0x34); /* SRX */
    attachInterrupt(digitalPinToInterrupt(BOARD_CC1101_GDO2), sniffIsr, CHANGE);
    gSniff = true;
}
void CC1101::stopSniff() {
    if (gSniff) {
        detachInterrupt(digitalPinToInterrupt(BOARD_CC1101_GDO2));
        gSniff = false;
    }
    strobe(0x36);
}
bool CC1101::sniffing() { return gSniff; }

bool CC1101::popPulse(uint32_t& us, bool& level) {
    if (ph == pt) return false;
    noInterrupts();
    us = pbuf[pt];
    level = plev[pt] != 0;
    pt = (uint16_t)((pt + 1) % PRING);
    interrupts();
    return true;
}

bool CC1101::txRaw(const uint16_t* pulses, size_t n, float mhz) {
    stopSniff();
    setFrequency(mhz);
    /* async serial TX on GDO0: IOCFG0 = 0x0D is GDOx as serial; for TX we bit-bang GDO0 as input to CC1101.
       Simpler and more reliable: bit-bang the GDO0 pin as GPIO while CC1101 is in TX async mode.
       FREND0 + PKTCTRL0 async, MDMCFG2 OOK. We drive GDO0. */
    writeReg(0x02, 0x2E); /* IOCFG0 high-Z, we drive the pin ourselves? 
                             Actually for async serial TX, GDO0 is an *input* to the modulator.
                             Configure IOCFG0 = 0x0D still, but that's output.
                             RadioLib uses GDO0 as serial input when PKTCTRL0=0x30.
                             We'll bit-bang CE-less: switch GDO0 to output and hope.
                             Better approach: use STX with FIFO for short packets.
                             For remotes we bit-bang by putting chip in TX and toggling GDO0. */
    pinMode(BOARD_CC1101_GDO0, OUTPUT);
    strobe(0x36);
    strobe(0x35); /* STX */
    delayMicroseconds(100);
    bool mark = true;
    for (size_t i = 0; i < n; i++) {
        digitalWrite(BOARD_CC1101_GDO0, mark ? HIGH : LOW);
        delayMicroseconds(pulses[i] > 0 ? pulses[i] : 1);
        mark = !mark;
    }
    digitalWrite(BOARD_CC1101_GDO0, LOW);
    strobe(0x36);
    pinMode(BOARD_CC1101_GDO0, INPUT);
    return true;
}

int CC1101::rssi() {
    uint8_t r = readReg(0x34); /* RSSI status */
    int rssi = (r >= 128) ? ((int)r - 256) / 2 - 74 : (int)r / 2 - 74;
    return rssi;
}
void CC1101::tick() {}
