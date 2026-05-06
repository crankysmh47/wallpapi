#pragma once
#include <windows.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <string>
#include <vector>

namespace wp {

struct ShaderUniforms {
    float iResolution[3];
    float iTime;
    float iTimeDelta;
    int   iFrame;
    float iMouse[4];     // xy: current, zw: click
    float iPrevMouse[4]; // xy: previous frame, zw: unused
};

class ShaderRenderer {
public:
    ShaderRenderer();
    ~ShaderRenderer();

    bool init(HWND hwnd, int width, int height);
    bool load_shader(const std::string& glsl_path);
    void render();
    void resize(int width, int height);
    void set_mouse(float x, float y, bool clicking);
    void cleanup();

private:
    bool create_device_and_swapchain(HWND hwnd);
    bool create_ping_pong_buffers();
    bool compile_shader(const std::string& glsl_source);
    bool compile_display_shader();

    HWND m_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapchain = nullptr;
    ID3D11RenderTargetView* m_rtv = nullptr;

    ID3D11VertexShader* m_vs = nullptr;
    ID3D11PixelShader* m_ps = nullptr;
    ID3D11PixelShader* m_ps_display = nullptr;
    ID3D11Buffer* m_uniform_buffer = nullptr;

    // Ping-pong simulation buffers (iChannel0)
    ID3D11Texture2D*          m_bufferTex[2] = {nullptr, nullptr};
    ID3D11RenderTargetView*   m_bufferRTV[2] = {nullptr, nullptr};
    ID3D11ShaderResourceView* m_bufferSRV[2] = {nullptr, nullptr};
    ID3D11SamplerState*       m_samplerState  = nullptr;
    
    int  m_frameIndex = 0;
    bool m_stateful = false; 

    ShaderUniforms m_uniforms = {};
    LARGE_INTEGER m_timer_frequency;
    LARGE_INTEGER m_start_time;
    LARGE_INTEGER m_last_frame_time;
};

} // namespace wp
