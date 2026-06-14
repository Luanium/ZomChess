cmake_minimum_required(VERSION 3.14)
project(ZomChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── 1. Source files (Dùng chung cho cả PC và Web) ───────────────────────────
set(ZOMCHESS_SOURCES
    src/main.cpp
    src/GameState.cpp
    src/AudioManager.cpp
)

# ─── 2. Tải thư viện Dear ImGui gốc (Dùng chung) ─────────────────────────────
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
)
FetchContent_MakeAvailable(imgui)

# Chỉ đường cho imgui-sfml biết vị trí của ImGui
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to ImGui top-level source" FORCE)

# ─── 3. CẤU HÌNH BIÊN DỊCH BẢN WEB (EMSCRIPTEN) ──────────────────────────────
if (EMSCRIPTEN)
    message(STATUS "Configuring for Emscripten / WebAssembly")

    # Ép CMake tìm kiếm SFML bản Web do GitHub Actions chuẩn bị sẵn
    find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

    set(IMGUI_SFML_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)

    FetchContent_Declare(
        imgui-sfml
        GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
        GIT_TAG        v2.6
    )
    FetchContent_MakeAvailable(imgui-sfml)

    add_executable(ZomChess ${ZOMCHESS_SOURCES})
    target_include_directories(ZomChess PRIVATE src)
    
    # Liên kết đầy đủ các module SFML Web và giao diện ImGui
    target_link_libraries(ZomChess PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        ImGui-SFML::ImGui-SFML
    )

    target_compile_options(ZomChess PRIVATE
        -sUSE_SDL=2
        -O2
    )

    set_target_properties(ZomChess PROPERTIES
        OUTPUT_NAME "ZomChess"
        SUFFIX ".js"
    )

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
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_BINARY_DIR}
            ${CMAKE_SOURCE_DIR}/web_build
        COMMENT "Copying web build artifacts to web_build/"
    )

# ─── 4. CẤU HÌNH BIÊN DỊCH BẢN PC NATIVE (Giữ nguyên cho máy Linux của bạn) ──
else()
    find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

    set(IMGUI_SFML_SHARED_LIBS ON CACHE BOOL "" FORCE)

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