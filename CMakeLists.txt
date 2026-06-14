cmake_minimum_required(VERSION 3.14)
project(ZomChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── 1. Danh sách file nguồn (Dùng chung) ────────────────────────────────────
set(ZOMCHESS_SOURCES
    src/main.cpp
    src/GameState.cpp
    src/AudioManager.cpp
)

include(FetchContent)

# ─── 2. Tải và cấu hình Dear ImGui (Dùng Populate để lấy đường dẫn chuẩn) ─────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# Cung cấp đường dẫn tuyệt đối chứa imgui.cpp cho imgui-sfml
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to ImGui top-level source" FORCE)

# ─── 3. CẤU HÌNH BIÊN DỊCH BẢN WEB (EMSCRIPTEN) ──────────────────────────────
if (EMSCRIPTEN)
    message(STATUS "Configuring for Emscripten / WebAssembly")

    # Tải bản SFML dành riêng cho WebAssembly trực tiếp qua CMake
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/Lautaro-Garcia/SFML.git
        GIT_TAG        master
    )
    set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
    set(SFML_BUILD_GRAPHICS ON CACHE BOOL "" FORCE)
    set(SFML_BUILD_WINDOW ON CACHE BOOL "" FORCE)
    set(SFML_BUILD_SYSTEM ON CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SFML)

    # Định nghĩa các Alias target để imgui-sfml có thể liên kết tự động
    add_library(SFML::System ALIAS sfml-system)
    add_library(SFML::Window ALIAS sfml-window)
    add_library(SFML::Graphics ALIAS sfml-graphics)
    add_library(SFML::Audio ALIAS sfml-audio)

    # Tắt tính năng tự tìm SFML bên ngoài của imgui-sfml vì ta đã nạp ở trên
    set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
    set(IMGUI_SFML_SHARED_LIBS OFF CACHE BOOL "" FORCE)

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

    target_compile_options(ZomChess PRIVATE
        -sUSE_SDL=2
        -O2
    )

    set_target_properties(ZomChess PROPERTIES
        OUTPUT_NAME "ZomChess"
        SUFFIX ".html"
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

# ─── 4. CẤU HÌNH BIÊN DỊCH BẢN PC (Giữ nguyên cho máy Linux local) ────────────
else()
    find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

    set(IMGUI_SFML_FIND_SFML ON CACHE BOOL "" FORCE)
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