#include <doctest/doctest.h>

#include "graphics.hpp"

#include <windows.h>

static HWND create_test_parent_window() {
    HINSTANCE hInst = GetModuleHandle(nullptr);
    const char* cls = "WallpapiTestParentWindow";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = hInst;
    wc.lpszClassName = cls;
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0, cls, "Wallpapi Test Parent",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        nullptr, nullptr, hInst, nullptr
    );
    return hwnd;
}

TEST_CASE("GraphicsEngine reinit destroys previous render window") {
    HWND parent1 = create_test_parent_window();
    REQUIRE(parent1 != nullptr);

    HWND parent2 = create_test_parent_window();
    REQUIRE(parent2 != nullptr);

    wp::GraphicsEngine ge;
    REQUIRE(ge.init(parent1));

    // After init, the render window should exist as a child.
    HWND child1 = FindWindowExA(parent1, nullptr, "WallpapiRenderWindow", nullptr);
    REQUIRE(child1 != nullptr);
    CHECK(IsWindow(child1));

    // Reinit to another host should destroy the old child and create a new one.
    REQUIRE(ge.reinit(parent2));

    CHECK(!IsWindow(child1));
    HWND child2 = FindWindowExA(parent2, nullptr, "WallpapiRenderWindow", nullptr);
    REQUIRE(child2 != nullptr);
    CHECK(IsWindow(child2));

    ge.cleanup();
    DestroyWindow(parent1);
    DestroyWindow(parent2);
}

