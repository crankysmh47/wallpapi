#define NOMINMAX
#include "../src/ipc_client.hpp"
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace {

constexpr int kWidth = 380;
constexpr int kHeight = 520;
constexpr UINT WM_APP_REFRESH = WM_APP + 1;

HWND g_hwnd = nullptr;
HWND g_status = nullptr;
HWND g_list = nullptr;
HWND g_btn_pause = nullptr;
HWND g_btn_add = nullptr;
HWND g_btn_open = nullptr;
HWND g_btn_next = nullptr;
HWND g_toggle_battery = nullptr;
HWND g_toggle_fullscreen = nullptr;
HWND g_toggle_muted = nullptr;

bool g_pause_on_battery = true;
bool g_pause_on_fullscreen = true;
bool g_muted = true;
bool g_paused = false;
std::wstring g_font_name = L"Segoe UI";

HFONT g_font = nullptr;
HFONT g_font_bold = nullptr;
HBRUSH g_bg_brush = nullptr;
COLORREF g_text = RGB(28, 28, 30);
COLORREF g_subtext = RGB(99, 99, 102);
COLORREF g_accent = RGB(0, 122, 255);

std::wstring to_wide(const std::string& s) {
    if (s.empty()) return {};
    const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    if (!out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

std::string to_utf8(const std::wstring& s) {
    if (s.empty()) return {};
    const int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, out.data(), len, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

std::string ipc_or_empty(const std::string& cmd) {
    std::string err;
    return wp::ipc_send(cmd, &err);
}

void set_status(const std::wstring& text) {
    if (g_status) SetWindowTextW(g_status, text.c_str());
}

bool parse_flag(const std::string& resp, const char* key, bool* out) {
    const std::string needle = std::string(key) + "=";
    const auto pos = resp.find(needle);
    if (pos == std::string::npos) return false;
    const auto start = pos + needle.size();
    const auto end = resp.find_first_of(" \n", start);
    const auto val = resp.substr(start, end == std::string::npos ? std::string::npos : end - start);
    if (val == "true") { *out = true; return true; }
    if (val == "false") { *out = false; return true; }
    return false;
}

void update_toggle(HWND btn, bool on) {
    if (!btn) return;
    SetWindowTextW(btn, on ? L"On" : L"Off");
}

void refresh_config() {
    const auto resp = ipc_or_empty("config get");
    if (!wp::ipc_ok(resp)) {
        set_status(L"Engine not running — start wp-engine first");
        return;
    }
    parse_flag(resp, "pause_on_battery", &g_pause_on_battery);
    parse_flag(resp, "pause_on_fullscreen", &g_pause_on_fullscreen);
    parse_flag(resp, "muted", &g_muted);
    update_toggle(g_toggle_battery, g_pause_on_battery);
    update_toggle(g_toggle_fullscreen, g_pause_on_fullscreen);
    update_toggle(g_toggle_muted, g_muted);
}

void refresh_status() {
    const auto resp = ipc_or_empty("status");
    if (!wp::ipc_ok(resp)) {
        set_status(L"Engine not running");
        return;
    }
    parse_flag(resp, "paused", &g_paused);
    SetWindowTextW(g_btn_pause, g_paused ? L"Resume" : L"Pause");
    set_status(g_paused ? L"Paused" : L"Playing");
}

void refresh_wallpapers() {
    if (!g_list) return;
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    const auto resp = ipc_or_empty("list");
    if (!wp::ipc_ok(resp)) return;

    size_t start = resp.find('\n');
    if (start == std::string::npos) return;
    ++start;
    while (start < resp.size()) {
        auto end = resp.find('\n', start);
        if (end == std::string::npos) end = resp.size();
        const std::string line = resp.substr(start, end - start);
        if (!line.empty()) {
            const auto w = to_wide(line);
            SendMessageW(g_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(w.c_str()));
        }
        start = end + 1;
    }
}

void refresh_all() {
    refresh_config();
    refresh_status();
    refresh_wallpapers();
}

bool apply_selected_wallpaper() {
    const int idx = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (idx == LB_ERR) return false;
    const int len = static_cast<int>(SendMessageW(g_list, LB_GETTEXTLEN, idx, 0));
    if (len <= 0) return false;
    std::wstring wpath(len + 1, L'\0');
    SendMessageW(g_list, LB_GETTEXT, idx, reinterpret_cast<LPARAM>(wpath.data()));
    wpath.resize(len);
    const auto cmd = std::string("set ") + to_utf8(wpath);
    const auto resp = ipc_or_empty(cmd);
    if (wp::ipc_ok(resp)) {
        set_status(L"Wallpaper updated");
        refresh_status();
        return true;
    }
    set_status(L"Failed to apply wallpaper");
    return false;
}

void toggle_ipc(const char* cmd) {
    const auto resp = ipc_or_empty(cmd);
    if (!wp::ipc_ok(resp)) {
        set_status(L"Could not update setting");
        return;
    }
    refresh_config();
    refresh_status();
}

void apply_acrylic(HWND hwnd) {
    const BOOL dark = FALSE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark)); // DWMWA_USE_IMMERSIVE_DARK_MODE

    enum { DWMSBT_MAINWINDOW = 2, DWMSBT_TRANSIENTWINDOW = 3 };
    const int backdrop = DWMSBT_TRANSIENTWINDOW;
    DwmSetWindowAttribute(hwnd, 38, &backdrop, sizeof(backdrop)); // DWMWA_SYSTEMBACKDROP_TYPE

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

HFONT make_font(int height, bool bold) {
    return CreateFontW(
        height, 0, 0, 0,
        bold ? FW_SEMIBOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        g_font_name.c_str()
    );
}

void create_controls(HWND hwnd) {
    const int pad = 20;
    int y = pad;

    CreateWindowExW(
        0, L"STATIC", L"Wallpapi",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        pad, y, kWidth - pad * 2, 24,
        hwnd, nullptr, nullptr, nullptr
    );
    y += 30;

    g_status = CreateWindowExW(
        0, L"STATIC", L"Connecting…",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        pad, y, kWidth - pad * 2, 18,
        hwnd, nullptr, nullptr, nullptr
    );
    y += 28;

    auto make_row = [&](const wchar_t* label, HWND* toggle, int row_y) {
        CreateWindowExW(0, L"STATIC", label, WS_CHILD | WS_VISIBLE | SS_LEFT,
            pad, row_y, 220, 20, hwnd, nullptr, nullptr, nullptr);
        *toggle = CreateWindowExW(
            0, L"BUTTON", L"On",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            kWidth - pad - 72, row_y - 2, 72, 26,
            hwnd, nullptr, nullptr, nullptr
        );
    };

    make_row(L"Pause on battery", &g_toggle_battery, y);
    y += 34;
    make_row(L"Pause on fullscreen", &g_toggle_fullscreen, y);
    y += 34;
    make_row(L"Mute audio", &g_toggle_muted, y);
    y += 40;

    g_btn_pause = CreateWindowExW(
        0, L"BUTTON", L"Pause",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        pad, y, (kWidth - pad * 2 - 8) / 2, 30,
        hwnd, reinterpret_cast<HMENU>(1001), nullptr, nullptr
    );
    g_btn_next = CreateWindowExW(
        0, L"BUTTON", L"Next",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        pad + (kWidth - pad * 2 - 8) / 2 + 8, y, (kWidth - pad * 2 - 8) / 2, 30,
        hwnd, reinterpret_cast<HMENU>(1002), nullptr, nullptr
    );
    y += 42;

    CreateWindowExW(0, L"STATIC", L"Wallpapers", WS_CHILD | WS_VISIBLE | SS_LEFT,
        pad, y, 200, 18, hwnd, nullptr, nullptr, nullptr);
    y += 22;

    g_list = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP,
        pad, y, kWidth - pad * 2, 200,
        hwnd, reinterpret_cast<HMENU>(1003), nullptr, nullptr
    );
    y += 212;

    const int btn_w = (kWidth - pad * 2 - 8) / 2;
    g_btn_add = CreateWindowExW(
        0, L"BUTTON", L"Add wallpaper…",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        pad, y, btn_w, 30,
        hwnd, reinterpret_cast<HMENU>(1004), nullptr, nullptr
    );
    g_btn_open = CreateWindowExW(
        0, L"BUTTON", L"Open folder",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        pad + btn_w + 8, y, btn_w, 30,
        hwnd, reinterpret_cast<HMENU>(1005), nullptr, nullptr
    );

    EnumChildWindows(hwnd, [](HWND child, LPARAM) -> BOOL {
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
        return TRUE;
    }, 0);
}

void add_wallpaper_dialog(HWND hwnd) {
    wchar_t file[MAX_PATH] = {};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Media\0*.mp4;*.mkv;*.webm;*.mov;*.avi;*.jpg;*.jpeg;*.png;*.webp\0All\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Add wallpaper";
    if (!GetOpenFileNameW(&ofn)) return;

    const auto path = to_utf8(file);
    const auto resp = ipc_or_empty(std::string("add ") + path);
    if (wp::ipc_ok(resp)) {
        set_status(L"Wallpaper added");
        refresh_all();
    } else {
        set_status(L"Failed to add wallpaper");
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_hwnd = hwnd;
            g_font = make_font(-15, false);
            g_font_bold = make_font(-17, true);
            g_bg_brush = CreateSolidBrush(RGB(245, 245, 247));
            create_controls(hwnd);
            apply_acrylic(hwnd);
            PostMessageW(hwnd, WM_APP_REFRESH, 0, 0);
            return 0;

        case WM_APP_REFRESH:
            refresh_all();
            return 0;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_subtext);
            return reinterpret_cast<LRESULT>(g_bg_brush);
        }
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetBkColor(hdc, RGB(255, 255, 255));
            SetTextColor(hdc, g_text);
            static HBRUSH list_brush = CreateSolidBrush(RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(list_brush);
        }

        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            const int code = HIWORD(wParam);
            if (code == BN_CLICKED) {
                if (id == 1001) {
                    const auto resp = ipc_or_empty(g_paused ? "resume" : "pause");
                    if (wp::ipc_ok(resp)) refresh_status();
                    return 0;
                }
                if (id == 1002) {
                    const auto resp = ipc_or_empty("next");
                    if (wp::ipc_ok(resp)) {
                        set_status(L"Next wallpaper");
                        refresh_all();
                    }
                    return 0;
                }
                if (id == 1004) {
                    add_wallpaper_dialog(hwnd);
                    return 0;
                }
                if (id == 1005) {
                    ipc_or_empty("open");
                    return 0;
                }
                if (reinterpret_cast<HWND>(lParam) == g_toggle_battery) {
                    toggle_ipc("toggle pause_on_battery");
                    return 0;
                }
                if (reinterpret_cast<HWND>(lParam) == g_toggle_fullscreen) {
                    toggle_ipc("toggle pause_on_fullscreen");
                    return 0;
                }
                if (reinterpret_cast<HWND>(lParam) == g_toggle_muted) {
                    toggle_ipc("toggle muted");
                    return 0;
                }
            }
            if (code == LBN_DBLCLK && reinterpret_cast<HWND>(lParam) == g_list) {
                apply_selected_wallpaper();
                return 0;
            }
            return 0;
        }

        case WM_DESTROY:
            if (g_font) DeleteObject(g_font);
            if (g_font_bold) DeleteObject(g_font_bold);
            if (g_bg_brush) DeleteObject(g_bg_brush);
            PostQuitMessage(0);
            return 0;

        case WM_ERASEBKGND: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(reinterpret_cast<HDC>(wParam), &rc, g_bg_brush);
            return 1;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    const wchar_t* cls = L"WallpapiUI";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = cls;
    wc.hbrBackground = nullptr;
    RegisterClassExW(&wc);

    const int sx = GetSystemMetrics(SM_CXSCREEN);
    const int sy = GetSystemMetrics(SM_CYSCREEN);
    const int x = (sx - kWidth) / 2;
    const int y = (sy - kHeight) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED,
        cls, L"Wallpapi",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, kWidth, kHeight,
        nullptr, nullptr, inst, nullptr
    );

    SetLayeredWindowAttributes(hwnd, 0, 245, LWA_ALPHA);
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
