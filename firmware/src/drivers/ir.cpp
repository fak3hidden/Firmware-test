#include "ir.h"
#include "../board.h"
#include "driver/rmt.h"

static constexpr rmt_channel_t TXCH = RMT_CHANNEL_0;
static constexpr rmt_channel_t RXCH = RMT_CHANNEL_1;
static bool gInit = false;

void IR::init() {
    if (gInit) return;
    pinMode(BOARD_IR_TX, OUTPUT);
    digitalWrite(BOARD_IR_TX, LOW);
    pinMode(BOARD_IR_RX, INPUT);

    rmt_config_t tx = {};
    tx.rmt_mode = RMT_MODE_TX;
    tx.channel = TXCH;
    tx.gpio_num = (gpio_num_t)BOARD_IR_TX;
    tx.mem_block_num = 2;
    tx.clk_div = 80; /* 1 us ticks */
    tx.tx_config.loop_en = false;
    tx.tx_config.carrier_en = true;
    tx.tx_config.carrier_freq_hz = 38000;
    tx.tx_config.carrier_duty_percent = 33;
    tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
    tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    tx.tx_config.idle_output_en = true;
    rmt_config(&tx);
    rmt_driver_install(TXCH, 0, 0);

    rmt_config_t rx = {};
    rx.rmt_mode = RMT_MODE_RX;
    rx.channel = RXCH;
    rx.gpio_num = (gpio_num_t)BOARD_IR_RX;
    rx.mem_block_num = 4;
    rx.clk_div = 80;
    rx.rx_config.filter_en = true;
    rx.rx_config.filter_ticks_thresh = 100;
    rx.rx_config.idle_threshold = 15000;
    rmt_config(&rx);
    rmt_driver_install(RXCH, 2048, 0);
    gInit = true;
}

int IR::capture(uint16_t* buf, int maxn, uint32_t timeoutMs) {
    if (!gInit) init();
    rmt_rx_start(RXCH, true);
    RingbufHandle_t rb = nullptr;
    rmt_get_ringbuf_handle(RXCH, &rb);
    size_t len = 0;
    rmt_item32_t* items = (rmt_item32_t*)xRingbufferReceive(rb, &len, pdMS_TO_TICKS(timeoutMs));
    int n = 0;
    if (items) {
        int count = (int)(len / sizeof(rmt_item32_t));
        for (int i = 0; i < count && n + 1 < maxn; i++) {
            buf[n++] = items[i].duration0;
            buf[n++] = items[i].duration1;
        }
        vRingbufferReturnItem(rb, items);
    }
    rmt_rx_stop(RXCH);
    return n;
}

static void fill_item(rmt_item32_t& it, uint16_t mark, uint16_t space) {
    it.level0 = 1; it.duration0 = mark;
    it.level1 = 0; it.duration1 = space;
}

bool IR::sendNEC(uint32_t data, uint8_t nbits) {
    if (!gInit) init();
    rmt_item32_t items[34];
    fill_item(items[0], 9000, 4500);
    for (int i = 0; i < nbits; i++) {
        bool bit = (data >> (nbits - 1 - i)) & 1;
        fill_item(items[1 + i], 560, bit ? 1690 : 560);
    }
    fill_item(items[1 + nbits], 560, 0);
    rmt_write_items(TXCH, items, nbits + 2, true);
    rmt_wait_tx_done(TXCH, pdMS_TO_TICKS(200));
    return true;
}

bool IR::sendRaw(const uint16_t* buf, int n, uint16_t freqKHz) {
    if (!gInit) init();
    (void)freqKHz;
    int pairs = (n + 1) / 2;
    rmt_item32_t* items = (rmt_item32_t*)malloc(sizeof(rmt_item32_t) * (pairs + 1));
    if (!items) return false;
    for (int i = 0; i < pairs; i++) {
        uint16_t m = buf[i * 2];
        uint16_t s = (i * 2 + 1 < n) ? buf[i * 2 + 1] : 0;
        fill_item(items[i], m, s);
    }
    rmt_write_items(TXCH, items, pairs, true);
    rmt_wait_tx_done(TXCH, pdMS_TO_TICKS(500));
    free(items);
    return true;
}

bool IR::decodeNEC(const uint16_t* buf, int n, uint32_t& data) {
    if (n < 66) return false;
    if (buf[0] < 7000 || buf[0] > 11000) return false;
    uint32_t v = 0;
    for (int i = 0; i < 32; i++) {
        uint16_t space = buf[2 + i * 2 + 1];
        v <<= 1;
        if (space > 1000) v |= 1;
    }
    data = v;
    return true;
}
