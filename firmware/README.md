# FinOS firmware

Pixel-perfect **128×64** Flipper-style UI, integer-scaled onto the LilyGO
T-Embed CC1101 / Plus **170×320** ST7789 (orange bezel, 2× nearest neighbour).

## Boards

| env | Hardware |
| --- | --- |
| `t-embed-cc1101` | LilyGO T-Embed CC1101 |
| `t-embed-cc1101-plus` | Same + onboard nRF24L01 |

MCU: ESP32-S3, 16MB flash, 8MB OPI PSRAM. Pin map matches LilyGO `utilities.h`.

## Build

```bash
pip install platformio
cd firmware
pio run -e t-embed-cc1101-plus
```

Flash (device in download mode: **hold encoder knob, tap RESET**):

```bash
pio run -e t-embed-cc1101-plus -t upload
```

Or merge a 0x0 image for the web flasher / qFin:

```bash
python -m esptool --chip esp32s3 merge_bin -o finos.bin \
  --flash_mode qio --flash_freq 80m --flash_size 16MB \
  0x0 .pio/build/t-embed-cc1101-plus/bootloader.bin \
  0x8000 .pio/build/t-embed-cc1101-plus/partitions.bin \
  0xe000 $HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/t-embed-cc1101-plus/firmware.bin
```

## Controls

| Input | Mapping |
| --- | --- |
| Encoder rotate | Up / Down |
| Encoder press | OK |
| User key (side) | Back |
| Long encoder press | Pet Fin / extra |

## Regenerating assets

```bash
python3 tools/gen_assets.py
```

Needs Pillow + fontTools. Font: Micro 5, SIL OFL 1.1.

## USB protocol

Framed `TF\x01` plus a human CLI (`help`, `info`, `ls /ext`, …). See `src/protocol.cpp`.
Apps are JSON `.tapp` files dropped in `/ext/apps`.
