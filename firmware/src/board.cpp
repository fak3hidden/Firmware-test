#include "board.h"
#include "version.h"
#include <Preferences.h>

static SPIClass gSpi(HSPI);
static SemaphoreHandle_t gSpiMux;
static bool gNrf = false, gCc = false, gNfc = false, gSd = false;
static uint8_t gBatt = 100;

static void csHigh(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}

void Board::init() {
    pinMode(BOARD_PWR_EN, OUTPUT);
    digitalWrite(BOARD_PWR_EN, HIGH);
    delay(20);

    csHigh(DISPLAY_CS);
    csHigh(BOARD_CC1101_CS);
    csHigh(BOARD_NRF24_CS);
    csHigh(BOARD_SD_CS);
    pinMode(BOARD_NRF24_CE, OUTPUT);
    digitalWrite(BOARD_NRF24_CE, LOW);
    pinMode(BOARD_CC1101_SW0, OUTPUT);
    pinMode(BOARD_CC1101_SW1, OUTPUT);
    pinMode(DISPLAY_BL, OUTPUT);
    digitalWrite(DISPLAY_BL, HIGH);
    pinMode(BOARD_PN532_RF_REST, OUTPUT);
    digitalWrite(BOARD_PN532_RF_REST, HIGH);
    pinMode(BOARD_PN532_IRQ, INPUT);
    pinMode(BOARD_USER_KEY, INPUT_PULLUP);

    gSpiMux = xSemaphoreCreateMutex();
    gSpi.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI, -1);
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
    Wire.setClock(100000);

    /* Probe peripherals once at boot. */
    delay(30);
    gNfc = probeI2C(BOARD_PN532_ADDR);
    /* CC1101: read version register 0x31 | 0xC0 (status+burst) -> version is 0x14 typically */
    deselectAll();
    setSpiHz(4000000);
    digitalWrite(BOARD_CC1101_CS, LOW);
    delayMicroseconds(10);
    gSpi.transfer(0x31 | 0xC0); /* VERSION, burst read */
    uint8_t ver = gSpi.transfer(0x00);
    digitalWrite(BOARD_CC1101_CS, HIGH);
    gCc = (ver != 0x00 && ver != 0xFF);

    deselectAll();
    digitalWrite(BOARD_NRF24_CS, LOW);
    gSpi.transfer(0x07); /* STATUS read (cmd 0x07 is actually R_REGISTER|STATUS = 0x07) */
    uint8_t st = gSpi.transfer(0x00);
    digitalWrite(BOARD_NRF24_CS, HIGH);
    gNrf = (st != 0x00 && st != 0xFF);
    deselectAll();

    pinMode(BOARD_SD_CS, OUTPUT);
    /* SD presence is confirmed later by Storage. */
}

SPIClass& Board::spi() { return gSpi; }

void Board::spiLock() { xSemaphoreTake(gSpiMux, portMAX_DELAY); }
void Board::spiUnlock() { xSemaphoreGive(gSpiMux); }

void Board::deselectAll() {
    digitalWrite(DISPLAY_CS, HIGH);
    digitalWrite(BOARD_CC1101_CS, HIGH);
    digitalWrite(BOARD_NRF24_CS, HIGH);
    digitalWrite(BOARD_SD_CS, HIGH);
}

void Board::selectDisplay() {
    deselectAll();
    setSpiHz(40000000);
    digitalWrite(DISPLAY_CS, LOW);
}
void Board::selectRadio() {
    deselectAll();
    setSpiHz(4000000);
    digitalWrite(BOARD_CC1101_CS, LOW);
}
void Board::selectNrf() {
    deselectAll();
    setSpiHz(8000000);
    digitalWrite(BOARD_NRF24_CS, LOW);
}
void Board::selectSd() {
    deselectAll();
    setSpiHz(20000000);
    digitalWrite(BOARD_SD_CS, LOW);
}
void Board::setSpiHz(uint32_t hz) { gSpi.setFrequency(hz); }

void Board::setBacklight(uint8_t duty) {
    analogWrite(DISPLAY_BL, duty);
}

void Board::cc1101Path(float mhz) {
    pinMode(BOARD_CC1101_SW0, OUTPUT);
    pinMode(BOARD_CC1101_SW1, OUTPUT);
    if (mhz < 380.0f) {             /* 315 band */
        digitalWrite(BOARD_CC1101_SW1, HIGH);
        digitalWrite(BOARD_CC1101_SW0, LOW);
    } else if (mhz < 500.0f) {      /* 433 band */
        digitalWrite(BOARD_CC1101_SW1, HIGH);
        digitalWrite(BOARD_CC1101_SW0, HIGH);
    } else {                        /* 868/915 */
        digitalWrite(BOARD_CC1101_SW1, LOW);
        digitalWrite(BOARD_CC1101_SW0, HIGH);
    }
}

bool Board::probeI2C(uint8_t addr) {
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

uint8_t Board::batteryPercent() {
    /* BQ27220 RelativeStateOfCharge at command 0x2C. Fallback: USB=100. */
    Wire.beginTransmission(BOARD_BQ27220_ADDR);
    Wire.write(0x2C);
    if (Wire.endTransmission(false) == 0 && Wire.requestFrom((uint8_t)BOARD_BQ27220_ADDR, (uint8_t)2) == 2) {
        uint8_t lo = Wire.read();
        uint8_t hi = Wire.read();
        uint16_t v = lo | (hi << 8);
        if (v <= 100) gBatt = (uint8_t)v;
    }
    return gBatt;
}

bool Board::usbConnected() {
    return true; /* CDC is up if we are running this poll from host; refined in protocol. */
}
bool Board::sdPresent() { return gSd; }
bool Board::nrfPresent() { return gNrf || FINOS_PLUS; }
bool Board::cc1101Present() { return gCc; }
bool Board::nfcPresent() { return gNfc; }
bool Board::isPlus() { return FINOS_PLUS || gNrf; }

void Board_setSdPresent(bool v) { gSd = v; }
