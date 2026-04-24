cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(Wallpapi CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force static linking of the MSVC C++ runtime.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

# 1. toml11 (Header-only)
FetchContent_Declare(toml11
    GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(toml11)

# 2. Lua (with CMake wrapper)
FetchContent_Declare(lua
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG v5.4.6
)
FetchContent_MakeAvailable(lua)

# 3. spdlog (Header-only or Static)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

# 4. Sokol (Header-only)
FetchContent_Declare(sokol
    GIT_REPOSITORY https://github.com/floooh/sokol.git
    GIT_TAG master
)
FetchContent_MakeAvailable(sokol)

# Define wp-engine
add_executable(wp-engine 
    src/main.cpp 
    src/config.cpp 
    src/ipc.cpp 
    src/logger.cpp
    src/graphics.cpp
    src/monitor.cpp
    src/lua_engine.cpp
)

# Define wp-cli
add_executable(wp-cli tools/cli.cpp)

# Find libmpv
find_library(MPV_LIB 
    NAMES mpv libmpv mpv-2 libmpv-2 libmpv.dll.a
    PATHS ${CMAKE_SOURCE_DIR}/deps/lib 
    REQUIRED
)

# Dependencies & Includes
target_include_directories(wp-engine PRIVATE 
    ${sokol_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/deps/include
    ${lua_SOURCE_DIR}/lua-5.4.6/include
    ${lua_SOURCE_DIR}/lua-5.4.6/src
)

target_link_libraries(wp-engine PRIVATE 
    toml11::toml11 
    lua_static 
    spdlog::spdlog
    ${MPV_LIB}
    user32 
    gdi32 
    dwmapi
    d3d11
    dxgi
)

target_link_libraries(wp-cli PRIVATE spdlog::spdlog)
