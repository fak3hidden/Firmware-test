# Flipper-style Firmware for the LILYGO T-Embed CC1101

A Flipper Zero–inspired look for the stock `factory` firmware of the
[LILYGO T-Embed CC1101](https://github.com/Xinyuan-LilyGO/T-Embed-CC1101),
plus a one-click Windows updater in Flipper style.

![startup](previews/preview_startup.png)

## What it looks like

A purpose-built Flipper Zero interface, not just a recolor:

- **Desktop** — animated bobbing dolphin with a mood line, a status bar with
  session clock + battery SOC, device info and a hint bar. Any key opens the menu.
- **Main menu** — true Flipper layout: full-screen scrolling list of full-width
  rows with app icons, inverted black selection bar and a scrollbar.
- **Apps** — the whole factory test suite (Battery, CC1101, IR, Mic, NFC,
  nRF24, SD, WiFi, TFT, WS2812, Settings) re-themed as monochrome Flipper
  "apps" with an orange accent, header hairlines and hint footers.

![menu](previews/preview_menu_page1.png)
![menu page 2](previews/preview_menu_page2.png)

## What's in this repo

| File | Purpose |
| --- | --- |
| `update.bat` | Flipper-styled updater: **pulls this repo first on the `arena/01a0aafc-firmware-test` branch** (checks out that branch if needed, refreshes itself, the theme patch and previews, then relaunches with the new code), clones the upstream LILYGO repo if missing, pulls new sources, applies the theme, and can build + flash with PlatformIO. |
| `compile.bat` | Flipper-styled builder: applies the theme, compiles the `T_Embed_CC1101` env with PlatformIO (auto-installs it via pip if missing), then flashes over USB with port auto-detect and retries, and can open the serial monitor. |
| `flipper-style.patch` | The theme itself (`examples/factory/flipper_style.h` + restyled UI files), applied to the local clone. |
| `previews/` | Renderings of the actual theme draw calls (menu page 1/2, startup). |
| `tools/ui_mock/` | Dev tool: compiles the real UI headers against a mock canvas on a desktop PC and re-renders the preview PNGs — lets you tweak the UI without hardware. |

The upstream repository (~306 MB) is **not** vendored here — `update.bat`
clones it on demand and keeps it in sync.

## Usage (Windows)

1. Double-click `update.bat`.
2. It clones `Xinyuan-LilyGO/T-Embed-CC1101` next to itself (or pulls updates
   if you already cloned it), then applies `flipper-style.patch`.
3. When PlatformIO is installed (`pip install platformio`) it offers to build
   and then flash the `T_Embed_CC1101` environment over USB.

Or skip straight to hardware:

1. Double-click `compile.bat`.
2. It applies the theme, compiles the factory firmware, flashes the device
   (auto-detecting the COM port, with retries and a BOOT-button hint), and
   offers to open the serial monitor at 115200 baud.

## Usage (manual / Linux)

```sh
git clone https://github.com/Xinyuan-LilyGO/T-Embed-CC1101.git
cd T-Embed-CC1101
git apply ../flipper-style.patch
pio run -e T_Embed_CC1101 -t upload
```

## Regenerating the previews (no hardware needed)

```sh
cd tools/ui_mock
g++ -std=c++14 -o harness harness.cpp
./harness > ops.txt
python3 render.py ops.txt ../../previews   # needs: pip install pillow
```

The harness compiles `flipper_style.h`, `main_menu_ui.h` and `page_startup.h`
from the applied clone unmodified, then dumps every draw call — so the
previews are exactly what the firmware renders.

If a future upstream update conflicts with the patch, the updater uses
`git apply --3way` and falls back to the stock look with a warning if the
theme can no longer be applied cleanly.

## Note

`update.bat` never blocks on the theme: source updates always fast-forward;
the patch is only applied to your **working tree**, so upstream history stays
pristine and rebasable.
