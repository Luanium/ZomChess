#!/usr/bin/env bash
# build_and_test.sh
# Builds both the native and web versions of ZomChess, then launches them.
# Usage:  ./build_and_test.sh          — build + run both
#         ./build_and_test.sh native   — build + run native only
#         ./build_and_test.sh web      — build + run web only
#         ./build_and_test.sh build    — build both, don't launch

set -e  # exit on first error

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_NATIVE="$ROOT/build"
BUILD_WEB="$ROOT/build_web_build"   # cmake build dir
WEB_OUT="$ROOT/web_build"           # final html/wasm output
EMSDK="$HOME/emsdk/emsdk_env.sh"
WEB_PORT=8080

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
info()    { echo -e "${GREEN}[build]${NC} $*"; }
warn()    { echo -e "${YELLOW}[warn]${NC}  $*"; }
die()     { echo -e "${RED}[error]${NC} $*" >&2; exit 1; }

MODE="${1:-both}"  # both | native | web | build

# ── Build native ──────────────────────────────────────────────────────────────
build_native() {
    info "Building native..."
    mkdir -p "$BUILD_NATIVE"
    cd "$BUILD_NATIVE"
    cmake "$ROOT" -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF \
          --log-level=WARNING > /dev/null 2>&1 || true
    cmake --build . -j"$(nproc)"
    info "Native build done → $BUILD_NATIVE/ZomChess"
}

# ── Build web ─────────────────────────────────────────────────────────────────
build_web() {
    if [ ! -f "$EMSDK" ]; then
        die "Emscripten not found at $EMSDK. See BUILD_WEB.md for setup instructions."
    fi
    info "Activating Emscripten..."
    # shellcheck source=/dev/null
    source "$EMSDK"

    info "Building web (WASM)..."
    mkdir -p "$BUILD_WEB_BUILD"
    cd "$BUILD_WEB_BUILD"

    # Reconfigure only when CMakeLists.txt is newer than the cache
    if [ ! -f CMakeCache.txt ] || [ "$ROOT/CMakeLists.txt" -nt CMakeCache.txt ]; then
        info "Running emcmake cmake..."
        emcmake cmake "$ROOT" -DCMAKE_BUILD_TYPE=Release --log-level=WARNING > /dev/null 2>&1
    fi

    cmake --build . -j"$(nproc)"
    info "Web build done → $WEB_OUT/"
}

# ── Launch native ─────────────────────────────────────────────────────────────
launch_native() {
    local bin="$BUILD_NATIVE/ZomChess"
    [ -f "$bin" ] || die "Native binary not found: $bin"
    info "Launching native game..."
    cd "$BUILD_NATIVE"
    "$bin" &
    NATIVE_PID=$!
    info "Native PID: $NATIVE_PID"
}

# ── Launch web ────────────────────────────────────────────────────────────────
launch_web() {
    [ -f "$WEB_OUT/ZomChess.html" ] || die "Web build not found: $WEB_OUT/ZomChess.html"

    # Kill any previous server on this port
    fuser -k ${WEB_PORT}/tcp > /dev/null 2>&1 || true

    info "Starting HTTP server on http://localhost:${WEB_PORT} ..."
    cd "$WEB_OUT"
    python3 -c "
import http.server

class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()
    def log_message(self, fmt, *args): pass  # silence

http.server.HTTPServer(('', ${WEB_PORT}), Handler).serve_forever()
" &
    SERVER_PID=$!
    info "HTTP server PID: $SERVER_PID"

    sleep 1
    info "Opening browser → http://localhost:${WEB_PORT}/ZomChess.html"
    xdg-open "http://localhost:${WEB_PORT}/ZomChess.html" > /dev/null 2>&1 || \
    sensible-browser "http://localhost:${WEB_PORT}/ZomChess.html" > /dev/null 2>&1 || \
    warn "Could not auto-open browser. Visit: http://localhost:${WEB_PORT}/ZomChess.html"
}

# Resolve the cmake build dir variable name typo from the function above
BUILD_WEB_BUILD="$ROOT/build_web_cmake"

# ── Main ──────────────────────────────────────────────────────────────────────
case "$MODE" in
    native)
        build_native
        launch_native
        info "Press Ctrl+C to stop."
        wait $NATIVE_PID
        ;;
    web)
        build_web
        launch_web
        info "Press Ctrl+C to stop the web server."
        wait $SERVER_PID
        ;;
    build)
        build_native
        build_web
        info "Both builds complete. Run without arguments to also launch."
        ;;
    both|*)
        build_native
        build_web
        launch_native
        launch_web
        info "Both running. Press Ctrl+C to stop web server (native closes on its own)."
        # Wait for server; native game runs independently
        wait $SERVER_PID
        ;;
esac
