#include "shader_renderer.hpp"
#include "logger.hpp"
#include <d3dcompiler.h>
#include <fstream>
#include <sstream>
#include <chrono>

namespace wp {

const char* VS_SOURCE = R"(
struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT main(uint vI : SV_VertexID) {
    VS_OUTPUT output;
    output.uv = float2((vI << 1) & 2, vI & 2);
    output.pos = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}
)";

ShaderRenderer::ShaderRenderer() {
    QueryPerformanceFrequency(&m_timer_frequency);
    QueryPerformanceCounter(&m_start_time);
    m_last_frame_time = m_start_time;
}

ShaderRenderer::~ShaderRenderer() {
    cleanup();
}

bool ShaderRenderer::init(HWND hwnd, int width, int height) {
    m_hwnd = hwnd;
    m_width = width;
    m_height = height;

    if (!create_device_and_swapchain(hwnd)) return false;
    if (!create_ping_pong_buffers()) return false;
    if (!compile_display_shader()) return false;

    // Create Constant Buffer
    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.ByteWidth = (sizeof(ShaderUniforms) + 15) & ~15; // Align to 16 bytes
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    if (FAILED(m_device->CreateBuffer(&cb_desc, nullptr, &m_uniform_buffer))) {
        WP_ERROR("Failed to create uniform buffer");
        return false;
    }

    // Compile Vertex Shader
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* err_blob = nullptr;
    if (FAILED(D3DCompile(VS_SOURCE, strlen(VS_SOURCE), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vs_blob, &err_blob))) {
        WP_ERROR("Failed to compile internal vertex shader");
        return false;
    }
    m_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &m_vs);
    vs_blob->Release();

    return true;
}

bool ShaderRenderer::create_ping_pong_buffers() {
    for (int i = 0; i < 2; ++i) {
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = m_width;
        td.Height = m_height;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        if (FAILED(m_device->CreateTexture2D(&td, nullptr, &m_bufferTex[i]))) return false;
        if (FAILED(m_device->CreateRenderTargetView(m_bufferTex[i], nullptr, &m_bufferRTV[i]))) return false;
        if (FAILED(m_device->CreateShaderResourceView(m_bufferTex[i], nullptr, &m_bufferSRV[i]))) return false;
    }

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;

    if (FAILED(m_device->CreateSamplerState(&sd, &m_samplerState))) return false;

    return true;
}

bool ShaderRenderer::compile_display_shader() {
    const char* display_ps = R"(
        Texture2D iChannel0 : register(t0);
        SamplerState iChannel0Sampler : register(s0);
        struct VS_OUTPUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
        float4 main(VS_OUTPUT input) : SV_Target {
            float4 data = iChannel0.Sample(iChannel0Sampler, input.uv);
            float ink = data.w;
            float2 vel = data.xy;
            
            float3 col = lerp(float3(0.02, 0.05, 0.1), float3(0.0, 0.6, 1.0), ink);
            col += float3(0.1, 0.4, 0.8) * length(vel) * 2.0;
            return float4(col, 1.0);
        }
    )";

    ID3DBlob* ps_blob = nullptr;
    ID3DBlob* err_blob = nullptr;
    if (FAILED(D3DCompile(display_ps, strlen(display_ps), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &ps_blob, &err_blob))) {
        return false;
    }
    m_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &m_ps_display);
    ps_blob->Release();
    return true;
}

bool ShaderRenderer::create_device_and_swapchain(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = m_width;
    scd.BufferDesc.Height = m_height;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
    if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, feature_levels, 1, D3D11_SDK_VERSION, &scd, &m_swapchain, &m_device, nullptr, &m_context))) {
        WP_ERROR("Failed to create D3D11 device and swapchain");
        return false;
    }

    WP_INFO("D3D11 ShaderRenderer initialized: {}x{}", m_width, m_height);

    ID3D11Texture2D* back_buffer = nullptr;
    m_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
    m_device->CreateRenderTargetView(back_buffer, nullptr, &m_rtv);
    back_buffer->Release();

    return true;
}

bool ShaderRenderer::load_shader(const std::string& glsl_path) {
    std::ifstream compat_file("shaders/compat.hlsl");
    std::ifstream user_file(glsl_path);

    if (!user_file.is_open()) {
        WP_ERROR("Failed to open shader file: {}", glsl_path);
        return false;
    }

    std::string user_source((std::istreambuf_iterator<char>(user_file)), std::istreambuf_iterator<char>());
    m_stateful = (user_source.find("[STATEFUL]") != std::string::npos);

    std::stringstream ss;
    if (compat_file.is_open()) ss << compat_file.rdbuf() << "\n";
    ss << user_source;

    return compile_shader(ss.str());
}

bool ShaderRenderer::compile_shader(const std::string& hlsl_source) {
    ID3DBlob* ps_blob = nullptr;
    ID3DBlob* err_blob = nullptr;

    HRESULT hr = D3DCompile(hlsl_source.c_str(), hlsl_source.length(), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &ps_blob, &err_blob);
    
    if (FAILED(hr)) {
        if (err_blob) {
            WP_ERROR("Shader Compilation Error: {}", (const char*)err_blob->GetBufferPointer());
            err_blob->Release();
        }
        return false;
    }

    ID3D11PixelShader* new_ps = nullptr;
    m_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &new_ps);
    ps_blob->Release();

    if (m_ps) m_ps->Release();
    m_ps = new_ps;

    WP_INFO("Shader loaded successfully. Stateful: {}", m_stateful);
    return true;
}

void ShaderRenderer::render() {
    if (!m_ps || !m_rtv || m_width <= 0 || m_height <= 0) return;

    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    
    double elapsed = (double)(current_time.QuadPart - m_start_time.QuadPart) / m_timer_frequency.QuadPart;
    double delta = (double)(current_time.QuadPart - m_last_frame_time.QuadPart) / m_timer_frequency.QuadPart;
    m_last_frame_time = current_time;

    m_uniforms.iResolution[0] = (float)m_width;
    m_uniforms.iResolution[1] = (float)m_height;
    m_uniforms.iResolution[2] = 1.0f;
    m_uniforms.iTime = (float)elapsed;
    m_uniforms.iTimeDelta = (float)delta;
    m_uniforms.iFrame++;

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(m_context->Map(m_uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &m_uniforms, sizeof(ShaderUniforms));
        m_context->Unmap(m_uniform_buffer, 0);
    }

    D3D11_VIEWPORT vp = { 0, 0, (float)m_width, (float)m_height, 0.0f, 1.0f };
    m_context->RSSetViewports(1, &vp);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_vs, nullptr, 0);
    m_context->PSSetConstantBuffers(0, 1, &m_uniform_buffer);

    if (m_stateful) {
        int readIdx = m_frameIndex % 2;
        int writeIdx = (m_frameIndex + 1) % 2;

        // Pass 1: Simulation
        m_context->OMSetRenderTargets(1, &m_bufferRTV[writeIdx], nullptr);
        m_context->PSSetShaderResources(0, 1, &m_bufferSRV[readIdx]);
        m_context->PSSetSamplers(0, 1, &m_samplerState);
        m_context->PSSetShader(m_ps, nullptr, 0);
        m_context->Draw(3, 0);

        // Hazard unbind
        ID3D11ShaderResourceView* nullSRV = nullptr;
        m_context->PSSetShaderResources(0, 1, &nullSRV);

        // Pass 2: Display
        float clear_color[4] = { 0, 0, 0, 1 };
        m_context->ClearRenderTargetView(m_rtv, clear_color);
        m_context->OMSetRenderTargets(1, &m_rtv, nullptr);
        m_context->PSSetShaderResources(0, 1, &m_bufferSRV[writeIdx]);
        m_context->PSSetSamplers(0, 1, &m_samplerState);
        m_context->PSSetShader(m_ps_display, nullptr, 0);
        m_context->Draw(3, 0);

        m_frameIndex++;
    } else {
        float clear_color[4] = { 0, 0, 0, 1 };
        m_context->ClearRenderTargetView(m_rtv, clear_color);
        m_context->OMSetRenderTargets(1, &m_rtv, nullptr);
        m_context->PSSetShader(m_ps, nullptr, 0);
        m_context->Draw(3, 0);
    }

    m_swapchain->Present(1, 0);
}

void ShaderRenderer::resize(int width, int height) {
    if (m_width == width && m_height == height) return;
    m_width = width;
    m_height = height;

    if (m_swapchain) {
        m_context->OMSetRenderTargets(0, nullptr, nullptr);
        m_rtv->Release();
        
        for (int i = 0; i < 2; ++i) {
            if (m_bufferRTV[i]) m_bufferRTV[i]->Release();
            if (m_bufferSRV[i]) m_bufferSRV[i]->Release();
            if (m_bufferTex[i]) m_bufferTex[i]->Release();
            m_bufferRTV[i] = nullptr;
            m_bufferSRV[i] = nullptr;
            m_bufferTex[i] = nullptr;
        }

        m_swapchain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, 0);
        
        ID3D11Texture2D* back_buffer = nullptr;
        m_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
        m_device->CreateRenderTargetView(back_buffer, nullptr, &m_rtv);
        back_buffer->Release();

        create_ping_pong_buffers();
    }
}

void ShaderRenderer::set_mouse(float x, float y, bool clicking) {
    // Store previous pos for velocity calculation in shaders
    m_uniforms.iPrevMouse[0] = m_uniforms.iMouse[0];
    m_uniforms.iPrevMouse[1] = m_uniforms.iMouse[1];

    m_uniforms.iMouse[0] = x;
    m_uniforms.iMouse[1] = (float)m_height - y;
    if (clicking) {
        m_uniforms.iMouse[2] = x;
        m_uniforms.iMouse[3] = (float)m_height - y;
    } else {
        m_uniforms.iMouse[2] = 0;
        m_uniforms.iMouse[3] = 0;
    }
}

void ShaderRenderer::cleanup() {
    if (m_vs) m_vs->Release();
    if (m_ps) m_ps->Release();
    if (m_ps_display) m_ps_display->Release();
    if (m_uniform_buffer) m_uniform_buffer->Release();
    if (m_rtv) m_rtv->Release();
    
    for (int i = 0; i < 2; ++i) {
        if (m_bufferRTV[i]) m_bufferRTV[i]->Release();
        if (m_bufferSRV[i]) m_bufferSRV[i]->Release();
        if (m_bufferTex[i]) m_bufferTex[i]->Release();
    }
    if (m_samplerState) m_samplerState->Release();

    if (m_swapchain) m_swapchain->Release();
    if (m_context) m_context->Release();
    if (m_device) m_device->Release();
}

} // namespace wp
