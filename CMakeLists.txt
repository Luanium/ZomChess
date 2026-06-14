cmake_minimum_required(VERSION 3.14)
project(ZomChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Source files (shared between native and Emscripten builds) ───────────────
set(ZOMCHESS_SOURCES
    src/main.cpp
    src/GameState.cpp
    src/AudioManager.cpp
)

# ─── Emscripten (WebAssembly) build ──────────────────────────────────────────
# Detected automatically when you invoke CMake via emcmake.
if (EMSCRIPTEN)
    message(STATUS "Configuring for Emscripten / WebAssembly")

    # Fetch Dear ImGui (same version as native build)
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.90.4
    )
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()

    # ImGui-SFML uses system SFML; under Emscripten we point it at the ports.
    set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)
    set(IMGUI_SFML_FIND_SFML OFF CACHE INTERNAL "")
    set(IMGUI_SFML_SHARED_LIBS OFF CACHE INTERNAL "")

    FetchContent_Declare(
        imgui-sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG        v2.6
    )
    FetchContent_MakeAvailable(imgui-sfml)

    add_executable(ZomChess ${ZOMCHESS_SOURCES})
    target_include_directories(ZomChess PRIVATE src)
    target_link_libraries(ZomChess PRIVATE ImGui-SFML::ImGui-SFML)

    # Emscripten ports: SFML depends on SDL2 under the hood when built via ports.
    # Use -sUSE_SDL=2 so Emscripten fetches & compiles SDL2 automatically.
    # SFML 2.x with Emscripten is typically provided via the emscripten-ports fork;
    # the flags below match that setup.
    target_compile_options(ZomChess PRIVATE
        -sUSE_SDL=2
        -O2
    )

    # Output file names: ZomChess.html / ZomChess.js / ZomChess.wasm
    set_target_properties(ZomChess PROPERTIES
        OUTPUT_NAME "ZomChess"
        SUFFIX ".html"
    )

    # --preload-file embeds the assets directory into the virtual MEMFS so the
    # browser can read them (music OGG files, font files, etc.).
    # Adjust the path if your working directory differs.
    target_link_options(ZomChess PRIVATE
        # Core runtime
        -sUSE_SDL=2
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=67108864     # 64 MB initial heap
        -sSTACK_SIZE=1048576          # 1 MB stack
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        -sNO_EXIT_RUNTIME=1

        # OpenAL (for SFML Audio)
        -lopenal

        # Embed assets into the virtual filesystem
        --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets

        # Use the default Emscripten shell as the HTML wrapper
        # (remove or replace with --shell-file to use a custom shell)
        # --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html

        # Full optimisation in release-style builds
        -O2
    )

    # Copy the generated web files to a dedicated output directory for clarity
    add_custom_command(TARGET ZomChess POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/web_build
        COMMENT "Copying web build artifacts to web_build/"
    )

# ─── Native (desktop) build ───────────────────────────────────────────────────
else()
    # 1. Find system SFML package
    find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

    # 2. Fetch the core Dear ImGui library first
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.90.4 # Stable version compatible with ImGui-SFML v2.6
    )
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)
    endif()

    # 3. Configure ImGui-SFML variables to point to the fetched ImGui core
    # and bypass internal SFML static search issues.
    set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to ImGui source code" FORCE)
    set(IMGUI_SFML_FIND_SFML OFF CACHE INTERNAL "Disable internal SFML search in ImGui-SFML")
    set(IMGUI_SFML_SHARED_LIBS ON CACHE INTERNAL "Force ImGui-SFML to link dynamically")

    # 4. Fetch ImGui-SFML wrapper
    FetchContent_Declare(
        imgui-sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG        v2.6
    )
    FetchContent_MakeAvailable(imgui-sfml)

    # 5. Build executable target
    add_executable(ZomChess ${ZOMCHESS_SOURCES})

    target_include_directories(ZomChess PRIVATE src)

    # 6. Link SFML audio component for music and sound effects
    target_link_libraries(ZomChess PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        ImGui-SFML::ImGui-SFML
    )

    # 7. Copy assets folder to build directory after build
    add_custom_command(TARGET ZomChess POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:ZomChess>/assets
        COMMENT "Copying assets to build directory"
    )
endif()
