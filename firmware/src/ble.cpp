#include "ble.h"
#include "board.h"
#include "protocol.h"
#include "input.h"
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string.h>

/* Nordic UART Service — Flipper-style serial over BLE for phone & PC. */
static const char* NUS_SVC = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char* NUS_RX  = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char* NUS_TX  = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

static BLEServer* gServer = nullptr;
static BLECharacteristic* gTx = nullptr;
static BLECharacteristic* gRx = nullptr;
static bool gOn = false, gConn = false, gInited = false;
static char gName[20] = "Flipper";
static volatile uint8_t rk = 0xFF, rt = 0;
static Preferences prefs;

class SrvCB : public BLEServerCallbacks {
    void onConnect(BLEServer*) override { gConn = true; }
    void onDisconnect(BLEServer* s) override {
        gConn = false;
        if (gOn) s->startAdvertising();
    }
};

class RxCB : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* ch) override {
        uint8_t* data = ch->getData();
        size_t n = ch->getLength();
        if (!data || !n) return;
        /* Remote buttons: "U" "D" "L" "R" "O" "B" as short presses,
         * or framed TF protocol passed to Protocol. */
        if (n == 1) {
            uint8_t k = 0xFF;
            switch (data[0]) {
                case 'U': case 'u': k = 0; break;
                case 'D': case 'd': k = 1; break;
                case 'L': case 'l': k = 2; break;
                case 'R': case 'r': k = 3; break;
                case 'O': case 'o': k = 4; break;
                case 'B': case 'b': k = 5; break;
            }
            if (k != 0xFF) { rk = k; rt = 2; /* Short */ return; }
        }
        Protocol::fromBle(data, n);
    }
};

void Ble::init() {
    prefs.begin("finos", false);
    String n = prefs.getString("ble_name", "Flipper");
    strncpy(gName, n.c_str(), sizeof(gName) - 1);
    gOn = prefs.getBool("ble_on", true);
    if (gOn) enable(true);
}

void Ble::enable(bool on) {
    gOn = on;
    prefs.putBool("ble_on", on);
    if (!on) {
        if (gInited) {
            BLEDevice::getAdvertising()->stop();
            gConn = false;
        }
        return;
    }
    if (!gInited) {
        BLEDevice::init(gName);
        gServer = BLEDevice::createServer();
        gServer->setCallbacks(new SrvCB());
        BLEService* svc = gServer->createService(NUS_SVC);
        gTx = svc->createCharacteristic(NUS_TX, BLECharacteristic::PROPERTY_NOTIFY);
        gTx->addDescriptor(new BLE2902());
        gRx = svc->createCharacteristic(
            NUS_RX, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
        gRx->setCallbacks(new RxCB());
        svc->start();
        BLEAdvertising* adv = BLEDevice::getAdvertising();
        adv->addServiceUUID(NUS_SVC);
        adv->setScanResponse(true);
        adv->setMinPreferred(0x06);
        BLEDevice::startAdvertising();
        gInited = true;
    } else {
        BLEDevice::startAdvertising();
    }
}

bool Ble::enabled() { return gOn; }
bool Ble::connected() { return gConn; }

void Ble::setName(const char* name) {
    strncpy(gName, name, sizeof(gName) - 1);
    gName[sizeof(gName) - 1] = 0;
    prefs.putString("ble_name", gName);
}

const char* Ble::name() { return gName; }

void Ble::forget() {
    gConn = false;
    if (gOn && gInited) {
        BLEDevice::startAdvertising();
    }
}

void Ble::poll() {}

bool Ble::popRemote(uint8_t& key, uint8_t& type) {
    if (rk == 0xFF) return false;
    key = rk; type = rt; rk = 0xFF;
    return true;
}

void Ble::send(const uint8_t* data, size_t n) {
    if (!gConn || !gTx || !data || !n) return;
    /* BLE notify MTU ~20-182. Chunk. */
    while (n) {
        size_t k = n > 120 ? 120 : n;
        gTx->setValue((uint8_t*)data, k);
        gTx->notify();
        data += k; n -= k;
    }
}
