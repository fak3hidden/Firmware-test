#!/usr/bin/env bash
# FinOS / qFin installer (Linux / macOS)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
echo "FinOS setup"
echo "  1) qFin desktop  2) flash firmware  3) web installer"
read -r -p "Choice [1/2/3]: " c
case "$c" in
  1)
    cd "$ROOT/desktop"
    npm install
    npm start
    ;;
  2)
    python3 "$ROOT/installer/setup.py"
    ;;
  *)
    python3 - <<'PY'
import webbrowser
webbrowser.open("https://fak3hidden.github.io/Firmware-test/install.html")
PY
    ;;
esac
