# FinOS installer

Three ways to put firmware on a T-Embed and install qFin.

## 1. Web wizard (easiest)

Open [Install](https://fak3hidden.github.io/Firmware-test/install.html) in Chrome or Edge.

1. Download qFin for your OS (or run `cd desktop && npm install && npm start`).
2. Hold the **encoder knob** and tap **RESET**.
3. Flash from the [web flasher](https://fak3hidden.github.io/Firmware-test/flash.html).
4. Pair a phone at [Phone](https://fak3hidden.github.io/Firmware-test/phone.html) — device name **Flipper**, Nordic UART.

## 2. Python GUI

```bash
python3 installer/setup.py
```

Installs qFin via npm and flashes a merged `.bin` with esptool (`pip install esptool`).

## 3. Packaged qFin

GitHub Actions (`Release` workflow on a `v*` tag) builds:

| File | What |
| --- | --- |
| `qFin-Setup.exe` | Windows NSIS installer |
| `qFin-mac.zip` | macOS |
| `qFin.AppImage` | Linux |
| `finos-t-embed-cc1101.bin` | Firmware (base) |
| `finos-t-embed-cc1101-plus.bin` | Firmware (Plus) |

From this tree without a release:

```bash
cd desktop && npm install && npm run dist
cd ../firmware && pio run
python3 installer/setup.py
```
