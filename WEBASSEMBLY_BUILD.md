# ZomChess — WebAssembly Build Guide

This document explains how to compile ZomChess to `.html` + `.js` + `.wasm` using
Emscripten, and what was changed in the source code to make it possible.

---

## Library Compatibility Summary

| Library | Compatible? | Notes |
|---|---|---|
| SFML Graphics / Window / System | ✅ Yes | Works via Emscripten's SDL2 backend |
| SFML Audio (`sf::Music`, `sf::Sound`) | ✅ Yes | Requires `-lopenal` (Emscripten bundles OpenAL Soft) |
| Dear ImGui | ✅ Yes | Pure C++, no platform dependencies |
| ImGui-SFML | ✅ Yes | Wrapper layer, compiles fine |
| `std::filesystem` | ❌ No | Not available in WASM; all usage is guarded with `#ifndef __EMSCRIPTEN__` |
| Procedural sound synthesis (`SoundSynth.h`) | ✅ Yes | Pure math, no OS calls |
| Embedded audio headers (`.h` binary blobs) | ✅ Yes | `loadFromMemory` works in WASM |

---

## What Changed in the Source Code

All changes use `#ifdef __EMSCRIPTEN__` / `#ifndef __EMSCRIPTEN__` guards so the
native desktop build is completely unaffected.

### `src/main.cpp`

1. **Headers** — `<filesystem>` and platform-specific headers (`<windows.h>`,
   `<mach-o/dyld.h>`) are only included on non-Emscripten builds.
   `<emscripten.h>` and `<emscripten/html5.h>` are included when targeting WASM.

2. **`get_exe_dir()`** — Returns an empty string on WASM (path resolution is
   meaningless in the browser's virtual filesystem).

3. **`FileBrowser` struct** — Entirely wrapped in `#ifndef __EMSCRIPTEN__` because
   it relies on `std::filesystem`. The Import/Export UI buttons are similarly
   guarded; a disabled-text placeholder is shown instead.

4. **Main loop refactoring** — This is the most important change. Browsers do not
   allow blocking `while` loops; the JavaScript event loop must stay in control.
   The solution is `emscripten_set_main_loop()`.

   All per-frame state that was previously inside `main()` (window, clocks, game
   state, UI flags, etc.) has been moved into a global `AppContext` struct. A
   free function `main_loop()` runs one frame at a time. `main()` sets everything
   up and then:
   - On **Emscripten**: calls `emscripten_set_main_loop(main_loop, 0, 1)` and
     returns immediately; the browser drives the loop.
   - On **Desktop**: calls `main_loop()` inside a `while(app.window.isOpen())`
     loop as before.

5. **Splash screen** — The original code used a separate inner `while` loop for
   the splash screen. This was converted to a two-phase state machine
   (`AppPhase::Splash` → `AppPhase::Game`) handled inside `main_loop()`.

6. **Font loading** — On WASM, font paths point to the virtual filesystem
   (`assets/fonts/...`) where fonts must be preloaded via `--preload-file`. On
   Linux/macOS the original system font paths are still used.

### `CMakeLists.txt`

A single file handles both builds via an `if(EMSCRIPTEN) … else() … endif()`
block. Key Emscripten flags:

| Flag | Purpose |
|---|---|
| `-sUSE_SDL=2` | Tells Emscripten to compile and link SDL2 (required by SFML's backend) |
| `-sALLOW_MEMORY_GROWTH=1` | Lets the WASM heap grow dynamically |
| `-sINITIAL_MEMORY=67108864` | 64 MB initial heap (enough for audio buffers) |
| `-lopenal` | Links Emscripten's bundled OpenAL Soft for `SFML Audio` |
| `--preload-file assets@/assets` | Embeds the `assets/` folder into the virtual MEMFS |
| `-O2` | Optimisation level |

---

## Prerequisites

### 1. Install Emscripten SDK

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # add to your shell profile for persistence
```

Verify: `emcc --version` should print something like `emcc (Emscripten ...) 3.x.x`.

### 2. Prepare fonts for the Web build

The web build cannot read system fonts from `/usr/share/fonts/...`. Copy at least
one font into `assets/fonts/`:

```bash
mkdir -p assets/fonts
cp /usr/share/fonts/truetype/noto/NotoSans-Bold.ttf assets/fonts/
# or any other TTF you have
```

If no font is found the splash/UI will render without text but won't crash.

### 3. (Optional) SFML Emscripten port

SFML 2.x has an unofficial Emscripten port. The most reliable way to use it with
CMake and `emscripten_set_main_loop` is through the
[emscripten-sfml](https://github.com/Lautaro-Garcia/SFML) fork or by letting
Emscripten's SDL2 layer provide the window/audio backend. ImGui-SFML v2.6 is
compatible with this setup.

> **Tip:** If SFML does not find its Emscripten port automatically, you can also
> build SFML yourself with `emcmake cmake` and point `SFML_DIR` to it.

---

## Building

### Native desktop (unchanged)

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
./ZomChess
```

### WebAssembly

```bash
# Activate the emsdk environment first (if not in your shell profile)
source /path/to/emsdk/emsdk_env.sh

mkdir build_web && cd build_web
emcmake cmake ..
cmake --build . -j$(nproc)
```

Output files will be in `build_web/` (and also copied to `web_build/`):

```
ZomChess.html   ← open this in a browser
ZomChess.js     ← JS glue code (loaded by the HTML)
ZomChess.wasm   ← the actual game logic
ZomChess.data   ← preloaded assets (music, fonts)
```

### Serving locally

Browsers block `file://` loading of `.wasm` files. Use a simple HTTP server:

```bash
# Python 3
cd build_web
python3 -m http.server 8080
# Then open http://localhost:8080/ZomChess.html
```

Or with Node.js:
```bash
npx serve build_web
```

---

## Known Limitations in the Web Version

- **File Import/Export (`.zom` challenge files)** — Disabled. The native file
  browser (`FileBrowser` struct) depends on `std::filesystem` which is not
  available in WASM. The UI shows a note explaining this. Future work could
  implement this using the browser's File System Access API via Emscripten's
  JavaScript binding.

- **Window title** — `sf::Style::Titlebar | sf::Style::Close` are silently
  ignored by Emscripten; the game renders in a `<canvas>` element.

- **Framerate cap** — `window.setFramerateLimit(60)` is not called on WASM
  builds; `requestAnimationFrame` inside the browser already targets the display
  refresh rate (typically 60 fps).

- **Audio autoplay policy** — Modern browsers block audio until the user
  interacts with the page. The splash screen requires a click/keypress to
  continue, which satisfies the browser's autoplay requirement before the game
  music starts.

---

## Troubleshooting

| Problem | Solution |
|---|---|
| `emcmake: command not found` | Run `source /path/to/emsdk/emsdk_env.sh` first |
| Blank canvas, no text | Font file not found in `assets/fonts/`; check the preload path |
| No audio | Make sure `-lopenal` is in the link flags and you clicked the page first (autoplay policy) |
| `RangeError: WebAssembly.Memory` | Increase `-sINITIAL_MEMORY` in CMakeLists.txt |
| SFML headers not found | Point `SFML_DIR` to your Emscripten-compiled SFML build |
| `std::filesystem` errors | You likely added a new file that includes `<filesystem>` without the guard; wrap it in `#ifndef __EMSCRIPTEN__` |
