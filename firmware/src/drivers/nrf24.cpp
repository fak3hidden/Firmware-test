#include "nrf24.h"
#include "../board.h"
#include <string.h>

static bool gOk = false;
static uint8_t gCh = 0;

static uint8_t rw(uint8_t a, uint8_t v) {
    Board::spiLock();
    Board::selectNrf();
    Board::spi().transfer(a);
    uint8_t r = Board::spi().transfer(v);
    Board::deselectAll();
    Board::spiUnlock();
    return r;
}
static uint8_t readReg(uint8_t r) { return rw((uint8_t)(0x00 | (r & 0x1F)), 0xFF); }
static void writeReg(uint8_t r, uint8_t v) { rw((uint8_t)(0x20 | (r & 0x1F)), v); }

bool NRF24::init() {
    pinMode(BOARD_NRF24_CE, OUTPUT);
    digitalWrite(BOARD_NRF24_CE, LOW);
    delay(5);
    writeReg(0x00, 0x08); /* CONFIG, PWR_UP=0 */
    uint8_t st = readReg(0x07);
    gOk = (st != 0x00 && st != 0xFF);
    if (!gOk) return false;
    writeReg(0x00, 0x0E); /* PWR_UP, CRC 2 byte */
    delay(5);
    writeReg(0x05, gCh);
    writeReg(0x06, 0x07); /* 1Mbps, 0dBm */
    writeReg(0x11, 32);   /* RX_PW_P0 */
    writeReg(0x01, 0x00); /* no auto-ack for scanner */
    writeReg(0x04, 0x00);
    return true;
}
bool NRF24::present() { return gOk; }
void NRF24::setChannel(uint8_t ch) { gCh = ch % 126; writeReg(0x05, gCh); }
uint8_t NRF24::channel() { return gCh; }

void NRF24::scan(uint8_t* hits, uint32_t dwellMs) {
    memset(hits, 0, 126);
    writeReg(0x00, 0x03); /* PWR_UP, PRIM_RX */
    for (uint8_t ch = 0; ch < 126; ch++) {
        writeReg(0x05, ch);
        digitalWrite(BOARD_NRF24_CE, HIGH);
        delay(dwellMs);
        uint8_t cd = readReg(0x09); /* CD */
        digitalWrite(BOARD_NRF24_CE, LOW);
        if (cd & 1) hits[ch]++;
    }
}

bool NRF24::tx(const uint8_t* data, uint8_t n) {
    if (n > 32) n = 32;
    writeReg(0x00, 0x0A); /* PWR_UP, PRIM_TX */
    Board::spiLock(); Board::selectNrf();
    Board::spi().transfer(0xE1); /* FLUSH_TX */
    Board::deselectAll();
    Board::selectNrf();
    Board::spi().transfer(0xA0); /* W_TX_PAYLOAD */
    for (uint8_t i = 0; i < n; i++) Board::spi().transfer(data[i]);
    Board::deselectAll(); Board::spiUnlock();
    digitalWrite(BOARD_NRF24_CE, HIGH);
    delayMicroseconds(15);
    digitalWrite(BOARD_NRF24_CE, LOW);
    delay(2);
    return true;
}

int NRF24::rx(uint8_t* data, uint8_t n, uint32_t timeoutMs) {
    writeReg(0x00, 0x0B); /* PWR_UP PRIM_RX */
    digitalWrite(BOARD_NRF24_CE, HIGH);
    uint32_t t0 = millis();
    while (millis() - t0 < timeoutMs) {
        uint8_t st = readReg(0x07);
        if (st & 0x40) { /* RX_DR */
            Board::spiLock(); Board::selectNrf();
            Board::spi().transfer(0x61);
            int i = 0;
            for (; i < n && i < 32; i++) data[i] = Board::spi().transfer(0xFF);
            Board::deselectAll(); Board::spiUnlock();
            writeReg(0x07, 0x40);
            digitalWrite(BOARD_NRF24_CE, LOW);
            return i;
        }
        delay(1);
    }
    digitalWrite(BOARD_NRF24_CE, LOW);
    return 0;
}
