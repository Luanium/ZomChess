# Building ZomChess for Web (itch.io)

## Prerequisites

### 1. Install Emscripten SDK
```bash
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
cd ~/emsdk
./emsdk install latest
./emsdk activate latest
source ~/emsdk/emsdk_env.sh   # add to ~/.bashrc to make permanent
```

### 2. Build Raylib for WebAssembly
```bash
cd ~/raylib/src
make PLATFORM=PLATFORM_WEB -j$(nproc)
# This produces libraylib.web.a in ~/raylib/src/
```

If `make` is not available for the web target, use:
```bash
cd ~/raylib/src
source ~/emsdk/emsdk_env.sh
emcc -c rcore.c rshapes.c rtextures.c rtext.c rmodels.c raudio.c \
     -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 \
     -I. -I../external/glfw/include
ar rcs libraylib.web.a rcore.o rshapes.o rtextures.o rtext.o rmodels.o raudio.o
```

## Building the Game

> **Note:** After any code change, always rebuild before testing.

```bash
cd /home/luan/Documents/GitHub/ZomChess

# Activate Emscripten (if not in ~/.bashrc)
source ~/emsdk/emsdk_env.sh

# Configure (only needed once, or after CMakeLists.txt changes)
mkdir -p build_web && cd build_web
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release

# Build (run this after every code change)
cmake --build . -j$(nproc)

# Output is copied to web_build/ automatically
cd ..
ls web_build/
# ZomChess.html  ZomChess.js  ZomChess.wasm  ZomChess.data
```

## Testing Locally Before Uploading

WebAssembly **cannot** be opened directly as a file (`file://`) — browsers block it
for security reasons. You must serve it through a local HTTP server.

```bash
cd /home/luan/Documents/GitHub/ZomChess/web_build
python3 -m http.server 8080
```

Then open your browser and go to:
```
http://localhost:8080/ZomChess.html
```

Press **Ctrl+C** in the terminal to stop the server when done.

### Common issues

**Black screen / console error "both async and sync fetching of the wasm failed"**
→ You opened the `.html` directly via `file://`. Use the Python server above.

**"RangeError: WebAssembly.Memory" or out-of-memory**
→ The game needs ~64 MB initial WASM memory. Make sure your browser is not
  in a restricted iframe. Running locally via the Python server avoids this.

**Game is tiny / wrong size**
→ The Emscripten shell uses a `<canvas>` that may not fill the browser window.
  This is cosmetic for local testing; itch.io lets you set the iframe size to
  1400×654 when you publish.

## Uploading to itch.io

1. Zip the four output files:
```bash
cd /home/luan/Documents/GitHub/ZomChess/web_build
zip ZomChess_itch.zip ZomChess.html ZomChess.js ZomChess.wasm ZomChess.data index.html
```

2. On itch.io project page:
   - Upload `ZomChess_web.zip`
   - Set **Kind of project**: HTML
   - Tick **"This file will be played in the browser"**
   - Set **Viewport dimensions**: 1400 × 654
   - Enable **"Mobile friendly"** only if you add touch controls later
