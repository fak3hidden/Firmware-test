#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

/* LilyGO T-Embed CC1101 / Plus pin map (official utilities.h). */

#define BOARD_USER_KEY  6
#define BOARD_PWR_EN    15

#define WS2812_NUM_LEDS 8
#define WS2812_DATA_PIN 14

#define BOARD_IR_EN     2
#define BOARD_IR_TX     2
#define BOARD_IR_RX     1

#define BOARD_MIC_DATA  42
#define BOARD_MIC_CLK   39
#define BOARD_VOICE_BCLK  46
#define BOARD_VOICE_LRCLK 40
#define BOARD_VOICE_DIN   7

#define DISPLAY_WIDTH   170
#define DISPLAY_HEIGHT  320
#define DISPLAY_BL      21
#define DISPLAY_CS      41
#define DISPLAY_MISO    10
#define DISPLAY_MOSI    9
#define DISPLAY_SCLK    11
#define DISPLAY_DC      16
#define DISPLAY_RST     -1

#define ENCODER_INA     4
#define ENCODER_INB     5
#define ENCODER_KEY     0

#define BOARD_I2C_SDA   8
#define BOARD_I2C_SCL   18

#define BOARD_PN532_ADDR    0x24
#define BOARD_PN532_RF_REST 45
#define BOARD_PN532_IRQ     17
#define BOARD_BQ27220_ADDR  0x55
#define BOARD_BQ25896_ADDR  0x6B

#define BOARD_SPI_SCK   11
#define BOARD_SPI_MOSI  9
#define BOARD_SPI_MISO  10

#define BOARD_NRF24_CS  44
#define BOARD_NRF24_CE  43

#define BOARD_SD_CS     13

#define BOARD_CC1101_CS   12
#define BOARD_CC1101_GDO0 3
#define BOARD_CC1101_GDO2 38
#define BOARD_CC1101_SW1  47
#define BOARD_CC1101_SW0  48

namespace Board {
    void init();
    SPIClass& spi();
    void spiLock();
    void spiUnlock();
    void deselectAll();
    void selectDisplay();
    void selectRadio();
    void selectNrf();
    void selectSd();
    void setSpiHz(uint32_t hz);
    void setBacklight(uint8_t duty); /* 0-255 */
    void cc1101Path(float mhz);
    bool probeI2C(uint8_t addr);
    uint8_t batteryPercent();
    bool usbConnected();
    bool sdPresent();
    bool nrfPresent();
    bool cc1101Present();
    bool nfcPresent();
    bool isPlus();
}
