cmake_minimum_required(VERSION 3.14)
project(ZomChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Source files (Raylib port) ───────────────────────────────────────────────
set(ZOMCHESS_SOURCES
    src/main_raylib.cpp
    src/GameState.cpp
)

# ─── Embed assets/guide.txt into a generated header ──────────────────────────
# cmake/EmbedGuide.cmake reads guide.txt and writes src/embedded/guide_txt.h
# containing the content as a raw string literal. Runs at build time so every
# rebuild picks up edits to the .txt file automatically.
set(GUIDE_INPUT  "${CMAKE_SOURCE_DIR}/assets/guide.txt")
set(GUIDE_OUTPUT "${CMAKE_SOURCE_DIR}/src/embedded/guide_txt.h")

add_custom_command(
    OUTPUT  "${GUIDE_OUTPUT}"
    COMMAND ${CMAKE_COMMAND}
            -DGUIDE_INPUT="${GUIDE_INPUT}"
            -DGUIDE_OUTPUT="${GUIDE_OUTPUT}"
            -P "${CMAKE_SOURCE_DIR}/cmake/EmbedGuide.cmake"
    DEPENDS "${GUIDE_INPUT}"
    COMMENT "Embedding assets/guide.txt into src/embedded/guide_txt.h"
)
add_custom_target(embed_guide ALL DEPENDS "${GUIDE_OUTPUT}")

# Fixed paths to a locally-built raylib (quick path for now; will be made
# portable via FetchContent later).
set(RAYLIB_NATIVE_INCLUDE "/usr/local/include")
set(RAYLIB_NATIVE_LIB     "/usr/local/lib/libraylib.a")
set(RAYLIB_WEB_INCLUDE    "$ENV{HOME}/raylib/src")
set(RAYLIB_WEB_LIB        "$ENV{HOME}/raylib/src/libraylib.web.a")

# ─── Emscripten (WebAssembly) build ───────────────────────────────────────────
if (EMSCRIPTEN)
    message(STATUS "Configuring for Emscripten / WebAssembly (Raylib)")

    add_executable(ZomChess ${ZOMCHESS_SOURCES})
    target_include_directories(ZomChess PRIVATE src ${RAYLIB_WEB_INCLUDE})
    target_compile_definitions(ZomChess PRIVATE RAYLIB_BUILD)
    target_compile_options(ZomChess PRIVATE -include ${CMAKE_SOURCE_DIR}/src/RaylibBuildPrelude.h)
    target_compile_options(ZomChess PRIVATE -O2)
    add_dependencies(ZomChess embed_guide)

    set_target_properties(ZomChess PROPERTIES OUTPUT_NAME "ZomChess" SUFFIX ".html")
    target_link_libraries(ZomChess PRIVATE ${RAYLIB_WEB_LIB})
    target_link_options(ZomChess PRIVATE
        -sUSE_GLFW=3
        -sALLOW_MEMORY_GROWTH=1
        -sMAXIMUM_MEMORY=536870912
        -sINITIAL_MEMORY=67108864
        -sSTACK_SIZE=1048576
        -sNO_EXIT_RUNTIME=1
        --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
        -O2
    )
    add_custom_command(TARGET ZomChess POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/web_build
        COMMENT "Copying web build artifacts to web_build/"
    )

# ─── Native (desktop) build ───────────────────────────────────────────────────
else()
    add_executable(ZomChess ${ZOMCHESS_SOURCES})
    target_include_directories(ZomChess PRIVATE src ${RAYLIB_NATIVE_INCLUDE})
    target_compile_definitions(ZomChess PRIVATE RAYLIB_BUILD)
    target_compile_options(ZomChess PRIVATE -include ${CMAKE_SOURCE_DIR}/src/RaylibBuildPrelude.h)
    add_dependencies(ZomChess embed_guide)

    target_link_libraries(ZomChess PRIVATE
        ${RAYLIB_NATIVE_LIB}
        GL
        m
        pthread
        dl
        rt
        X11
    )

    add_custom_command(TARGET ZomChess POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:ZomChess>/assets
        COMMENT "Copying assets to build directory"
    )

    # Lightweight target: sync only guide.txt without a full recompile.
    # Usage: cmake --build build --target sync-guide
    add_custom_target(sync-guide
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/assets/guide.txt
            $<TARGET_FILE_DIR:ZomChess>/assets/guide.txt
        COMMENT "Syncing guide.txt to build directory"
    )
endif()
