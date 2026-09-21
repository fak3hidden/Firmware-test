#pragma once
#include <stdint.h>
#include <stddef.h>

namespace CC1101 {
    bool init();
    bool present();
    void idle();
    void sleep();
    bool setFrequency(float mhz);
    float frequency();
    void setOok(bool ook);
    void setPower(int8_t dbm);
    /* RAW OOK sniff: GDO2 as async serial data. ISR fills pulse buffer. */
    void startSniff();
    void stopSniff();
    bool sniffing();
    /* pop next pulse duration in us, 0 if empty. high bit unused; sign via level? we store duration only. */
    bool popPulse(uint32_t& us, bool& level);
    /* transmit raw mark/space list, microseconds, starting with mark. */
    bool txRaw(const uint16_t* pulses, size_t n, float mhz);
    int rssi(); /* dBm */
    void tick();
}
