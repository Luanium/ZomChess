cmake_minimum_required(VERSION 3.14)
project(ZomChess)

# Yêu cầu chuẩn C++20 nghiêm ngặt
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# =================================================================
# 1. TẢI VÀ ÉP BIÊN DỊCH TĨNH SFML TỪ MÃ NGUỒN
# =================================================================
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build SFML as static libraries" FORCE)
set(SFML_BUILD_AUDIO ON CACHE BOOL "Enable audio component" FORCE)

FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.1
)
FetchContent_MakeAvailable(sfml)

# =================================================================
# 2. TẢI DEAR IMGUI
# =================================================================
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
)
FetchContent_MakeAvailable(imgui)

# =================================================================
# 3. TẢI IMGUI-SFML (CHỈ TẢI SOURCE, KHÔNG KÍCH HOẠT FILE CMAKE LỖI)
# =================================================================
FetchContent_Declare(
    imguisfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v2.6
)
FetchContent_Populate(imguisfml) # Lệnh này chặn đứng việc imguisfml tự ý chạy find_package

# =================================================================
# 4. GOM TẤT CẢ FILE NGUỒN ĐỂ ĐÚC RA FILE THỰC THI DUY NHẤT
# =================================================================
add_executable(ZomChess 
    main.cpp
    # Tự gom file .cpp của Dear ImGui để tự compile
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    # Tự gom file .cpp của bộ kết nối
    ${imguisfml_SOURCE_DIR}/imgui-SFML.cpp
)

# Khai báo đường dẫn chứa file Header (.h)
target_include_directories(ZomChess PRIVATE 
    ${imgui_SOURCE_DIR} 
    ${imguisfml_SOURCE_DIR}
)

# Liên kết với các target SFML tĩnh được sinh ra từ bước 1
target_link_libraries(ZomChess PRIVATE 
    sfml-graphics 
    sfml-window 
    sfml-system
    sfml-audio
)

# Vì build tĩnh SFML trên Linux, bắt buộc phải khai báo các thư viện nền của hệ thống để liên kết
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(ZomChess PRIVATE 
        GL pthread X11 Xrandr Xrender X11-xcb xcb xcb-randr xcb-image udev OpenGL
    )
endif()
