cmake_minimum_required(VERSION 3.14)
project(ZomChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Find system SFML package
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# 2. Fetch the core Dear ImGui library first
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4 # Stable version compatible with ImGui-SFML v2.6
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POP_PROV)
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
add_executable(ZomChess 
    src/main.cpp 
    src/GameState.cpp
    src/AudioManager.cpp
)

target_include_directories(ZomChess PRIVATE src)

# 7. Link SFML audio component for music and sound effects
target_link_libraries(ZomChess PRIVATE 
    sfml-graphics 
    sfml-window 
    sfml-system 
    sfml-audio
    ImGui-SFML::ImGui-SFML
)

# 8. Copy assets folder to build directory after build
add_custom_command(TARGET ZomChess POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:ZomChess>/assets
    COMMENT "Copying assets to build directory"
)
