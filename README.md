# FinOS

**Flipper-style firmware for the LilyGO T-Embed CC1101 and T-Embed CC1101 Plus.**

The real Flipper Zero screen is 128×64 monochrome. FinOS draws that exact canvas
(black ink on white, orange bezel) and integer-scales it 2× onto the T-Embed’s
1.9″ 170×320 colour panel. Chrome matches official firmware: **3-line framed
main menu**, 13px status bar, bottom action buttons, dolphin desktop.

Bluetooth uses the Nordic UART service (device name **Flipper**) so a phone or
PC can send the same buttons and file protocol as USB.

This is **not** Flipper firmware and is not affiliated with Flipper Devices.

## What’s in the tree

| Path | What |
| --- | --- |
| [`firmware/`](firmware/) | PlatformIO firmware (Arduino-ESP32, no third-party GUI libs) |
| [`docs/`](docs/) | GitHub Pages: landing, **install wizard**, **web flasher**, **phone BLE**, **app store** |
| [`desktop/`](desktop/) | **qFin** — Electron companion (qFlipper-shaped) + `qfin.py` CLI |
| [`installer/`](installer/) | Python/Tk setup GUI + shell wizard |
| [`docs/apps/`](docs/apps/) | `.tapp` catalog the store serves |

## Install (firmware + qFin)

1. Open **Install** on the Pages site, or `python3 installer/setup.py`.
2. Hold the **encoder knob** (BOOT / GPIO0) and tap **RESET**.
3. Flash Plus or CC1101 from the web flasher, qFin, or esptool.
4. On the device: Main menu → Bluetooth → On. Phone page talks Nordic UART.

Packaged Windows/macOS/Linux qFin builds come from the `Release` workflow
(`v*` tags): `qFin-Setup.exe`, `qFin-mac.zip`, `qFin.AppImage`.

Until CI has published `docs/firmware/*.bin`, choose a locally merged image.

## Install an app onto the device

With FinOS already running:

1. App store → **Connect device** → **Install to device**, or
2. `python3 desktop/qfin.py install docs/apps/dice.tapp`

`.tapp` files are JSON. The firmware copies them to `/ext/apps` and the
Applications menu runs them.

## qFin (desktop)

```bash
cd desktop
npm install
npm start          # Electron UI
# or
python3 qfin.py -p /dev/ttyACM0 info
```

## Build firmware

```bash
cd firmware
pip install platformio
pio run -e t-embed-cc1101-plus
```

See [`firmware/README.md`](firmware/README.md) for merge_bin, pins, protocol.

## Hardware used

ESP32-S3 · ST7789 170×320 · CC1101 Sub-GHz · PN532 NFC · IR TX/RX ·
WS2812 ring · rotary encoder · optional nRF24 (Plus) · microSD · 1300 mAh.

125 kHz RFID and iButton are **not** on this PCB; those menu entries say so.

## Legal

Use radios only on gear and frequencies you are allowed to use. FinOS ships
generic OOK capture/replay, NFC UID read, IR learn/send, and a HID text typer —
tools for **your** remotes, tags, and macros.

MIT. Micro 5 font: SIL OFL 1.1.
