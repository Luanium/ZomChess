cmake_minimum_required(VERSION 3.14)
project(ZomChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Cache dùng chung cho cả build Linux và build web ─────────────────────────
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/.fc_cache CACHE PATH "" FORCE)

# ─── Source files (shared between native and Emscripten builds) ───────────────
set(ZOMCHESS_SOURCES
    src/main.cpp
    src/GameState.cpp
    src/AudioManager.cpp
)

# ─── Emscripten (WebAssembly) build ──────────────────────────────────────────
if (EMSCRIPTEN)
    message(STATUS "Configuring for Emscripten / WebAssembly")

    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.90.4
    )
    FetchContent_MakeAvailable(imgui)

    FetchContent_Declare(
        imgui-sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG        v2.6
    )
    FetchContent_Populate(imgui-sfml)

    add_executable(ZomChess
        ${ZOMCHESS_SOURCES}
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui-sfml_SOURCE_DIR}/imgui-SFML.cpp
    )
    target_include_directories(ZomChess PRIVATE
        src
        ${imgui_SOURCE_DIR}
        ${imgui-sfml_SOURCE_DIR}
    )
    target_compile_definitions(ZomChess PRIVATE IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

    target_compile_options(ZomChess PRIVATE -sUSE_SDL=2 -O2)
    set_target_properties(ZomChess PROPERTIES OUTPUT_NAME "ZomChess" SUFFIX ".html")
    target_link_options(ZomChess PRIVATE
        -sUSE_SDL=2
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=67108864
        -sSTACK_SIZE=1048576
        -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']
        -sNO_EXIT_RUNTIME=1
        -lopenal
        --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
        -O2
    )
    add_custom_command(TARGET ZomChess POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/web_build
        COMMENT "Copying web build artifacts to web_build/"
    )

# ─── Native (desktop) build ───────────────────────────────────────────────────
else()
    find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.90.4
    )
    FetchContent_MakeAvailable(imgui)

    set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)
    set(IMGUI_SFML_FIND_SFML OFF CACHE INTERNAL "")
    set(IMGUI_SFML_SHARED_LIBS ON CACHE INTERNAL "")

    FetchContent_Declare(
        imgui-sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG        v2.6
    )
    FetchContent_MakeAvailable(imgui-sfml)

    add_executable(ZomChess ${ZOMCHESS_SOURCES})
    target_include_directories(ZomChess PRIVATE src)

    target_link_libraries(ZomChess PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        ImGui-SFML::ImGui-SFML
    )

    add_custom_command(TARGET ZomChess POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:ZomChess>/assets
        COMMENT "Copying assets to build directory"
    )
endif()
