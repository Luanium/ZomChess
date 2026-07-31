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

```bash
cd /home/luan/Documents/GitHub/ZomChess

# Activate Emscripten (if not in ~/.bashrc)
source ~/emsdk/emsdk_env.sh

# Configure
mkdir -p build_web && cd build_web
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Output is copied to web_build/ automatically
cd ..
ls web_build/
# ZomChess.html  ZomChess.js  ZomChess.wasm  ZomChess.data
```

## Uploading to itch.io

1. Zip the four output files:
```bash
cd web_build
zip ZomChess_web.zip ZomChess.html ZomChess.js ZomChess.wasm ZomChess.data
```

2. On itch.io project page:
   - Upload `ZomChess_web.zip`
   - Set **Kind of project**: HTML
   - Tick **"This file will be played in the browser"**
   - Set **Viewport dimensions**: 1400 × 654
   - Enable **"Mobile friendly"** only if you add touch controls later

## What's disabled on the web build

- **Export / Import .zom challenge files** — no filesystem on WASM
- **Guide hot-reload** — always uses the embedded guide compiled into the binary
- **Font loading from disk** — uses the embedded NotoSans-Bold.ttf

Everything else works identically to the native build.
