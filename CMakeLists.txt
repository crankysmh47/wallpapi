cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(Wallpapi CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

FetchContent_Declare(toml11
    GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(toml11)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

add_executable(wp-engine WIN32
    src/main.cpp
    src/config.cpp
    src/ipc.cpp
    src/logger.cpp
    src/graphics.cpp
    src/video_player.cpp
    src/monitor.cpp
    src/desktop_shell.cpp
)

add_executable(wp-cli tools/cli.cpp)

target_include_directories(wp-engine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(wp-engine PRIVATE
    toml11::toml11
    spdlog::spdlog
    mfplat
    mfuuid
    mfreadwrite
    user32
    gdi32
    shell32
    dwmapi
    ole32
    dxgi
)

add_custom_command(TARGET wp-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config.toml
        $<TARGET_FILE_DIR:wp-engine>/config.toml
)

add_custom_command(TARGET wp-engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:wp-engine>/wallpapers
)
