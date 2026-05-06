cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(Wallpapi CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prevent dependencies from registering their own tests (some embed cases
# create broken CTest entries, e.g. Lua's testsuite).
set(BUILD_TESTING OFF CACHE BOOL "Disable dependency tests" FORCE)

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

# 5. doctest (Single-header testing framework)
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

# Define wp-engine
add_executable(wp-engine WIN32
    src/main.cpp 
    src/config.cpp 
    src/ipc.cpp 
    src/logger.cpp
    src/graphics.cpp
    src/video_player.cpp
    src/monitor.cpp
    src/lua_engine.cpp
    src/shader_renderer.cpp
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
    shell32
    dwmapi
    d3d11
    dxgi
    d3dcompiler
)

target_link_libraries(wp-cli PRIVATE spdlog::spdlog)

# ----------------------------
# Tests
# ----------------------------
include(CTest)
enable_testing()

# Ensure any dependency-added tests don't break `ctest`.
if (TEST lua-testsuite)
    set_tests_properties(lua-testsuite PROPERTIES DISABLED TRUE)
endif()

# Also ignore it at CTest discovery time (covers cases where the test is
# registered with a bad working directory).
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/CTestCustom.cmake.in
    ${CMAKE_BINARY_DIR}/CTestCustom.cmake
    @ONLY
)

add_executable(wp-tests
    tests/main.cpp
    tests/test_config.cpp
    tests/test_ipc_commands.cpp
    tests/test_ipc_integration.cpp
    tests/test_fps_limiter.cpp
    tests/test_fullscreen_detect.cpp
    tests/test_graphics_reinit_win32.cpp
    tests/test_mpv_options.cpp
    tests/test_wallpaper_scan.cpp
    src/config.cpp
    src/logger.cpp
    src/ipc.cpp
    src/graphics.cpp
    src/video_player.cpp
    src/shader_renderer.cpp
)

target_include_directories(wp-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/deps/include
)

target_link_libraries(wp-tests PRIVATE
    toml11::toml11
    doctest::doctest
    spdlog::spdlog
    user32
    d3d11
    dxgi
    d3dcompiler
)

# Link libmpv to tests (needed by video_player.cpp)
find_library(MPV_LIB_TEST
    NAMES mpv libmpv mpv-2 libmpv-2 libmpv.dll.a
    PATHS ${CMAKE_SOURCE_DIR}/deps/lib
    REQUIRED
)
target_link_libraries(wp-tests PRIVATE ${MPV_LIB_TEST})

add_custom_command(TARGET wp-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/deps/lib/libmpv-2.dll
        $<TARGET_FILE_DIR:wp-tests>/libmpv-2.dll
)

add_test(NAME wp-tests COMMAND wp-tests)
