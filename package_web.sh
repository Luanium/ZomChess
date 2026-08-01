#!/usr/bin/env bash
# package_web.sh — Creates ZomChess_web.zip ready to distribute.
# Run after build_and_test.sh (web build must exist in web_build/).

set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/ZomChess_web.zip"
SRC="$ROOT/web_build"

[ -f "$SRC/ZomChess.html" ] || { echo "ERROR: web_build/ not found. Run ./build_and_test.sh web first."; exit 1; }

rm -f "$OUT"
cd "$SRC"
zip -q "$OUT" \
    index.html \
    ZomChess.html \
    ZomChess.js \
    ZomChess.wasm \
    ZomChess.data \
    play_game.py \
    play_game_windows.bat \
    play_game_linux_mac.sh

echo "Created: $OUT"
echo ""
echo "Contents:"
zip -sf "$OUT"
echo ""
echo "Instructions for recipients:"
echo "  Windows : double-click play_game_windows.bat"
echo "  Linux   : sh play_game_linux_mac.sh"
echo "  macOS   : sh play_game_linux_mac.sh  (or double-click in Finder)"
echo "  Manual  : python3 play_game.py"
