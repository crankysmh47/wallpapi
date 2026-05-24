#include "graphics.hpp"
#include "logger.hpp"

#include <mfapi.h>
#include <mfidl.h>
#include <mfmediaengine.h>
#include <mferror.h>
#include <dxgi.h>
#include <filesystem>
#include <cmath>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")

namespace wp {

namespace {

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return {};
    const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring out(static_cast<size_t>(len - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), len);
    return out;
}

class MediaEngineNotify final : public IMFMediaEngineNotify {
public:
    void attach(IMFMediaEngine* engine) {
        if (m_engine) {
            m_engine->Release();
            m_engine = nullptr;
        }
        m_engine = engine;
        if (m_engine) {
            m_engine->AddRef();
        }
        m_loop_reset = false;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == __uuidof(IMFMediaEngineNotify)) {
            *ppv = static_cast<IMFMediaEngineNotify*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_ref); }

    STDMETHODIMP_(ULONG) Release() override {
        const ULONG ref = InterlockedDecrement(&m_ref);
        if (ref == 0) delete this;
        return ref;
    }

    STDMETHODIMP EventNotify(DWORD event, DWORD_PTR param1, DWORD param2) override {
        switch (event) {
            case MF_MEDIA_ENGINE_EVENT_ERROR:
                WP_ERROR("Media Engine error code: {}", static_cast<unsigned>(param1));
                break;
            case MF_MEDIA_ENGINE_EVENT_LOADEDDATA:
            case MF_MEDIA_ENGINE_EVENT_CANPLAY:
                m_loaded = true;
                break;
            case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:
                seamless_loop_tick();
                break;
            case MF_MEDIA_ENGINE_EVENT_ENDED:
                // Fallback if the early seek missed the window.
                if (m_engine) {
                    m_engine->SetCurrentTime(0.0);
                    m_engine->Play();
                }
                break;
            default:
                break;
        }
        return S_OK;
    }

    bool loaded() const { return m_loaded; }
    void reset_loaded() { m_loaded = false; }

private:
    void seamless_loop_tick() {
        if (!m_engine) return;

        const double duration = m_engine->GetDuration();
        const double current = m_engine->GetCurrentTime();
        if (!std::isfinite(duration) || duration <= 0.0) return;

        // Seek to the start slightly before the last frame to avoid MF's loop gap.
        constexpr double kLoopLeadSec = 0.15;
        if (current >= duration - kLoopLeadSec) {
            if (!m_loop_reset) {
                m_engine->SetCurrentTime(0.0);
                m_loop_reset = true;
            }
            return;
        }

        if (current < 0.25) {
            m_loop_reset = false;
        }
    }

    ULONG m_ref = 1;
    bool m_loaded = false;
    bool m_loop_reset = false;
    IMFMediaEngine* m_engine = nullptr;
};

} // namespace

struct VideoPlayer::Impl {
    HWND hwnd = nullptr;
    IMFMediaEngine* engine = nullptr;
    MediaEngineNotify* notify = nullptr;

    ~Impl() {
        if (notify) {
            notify->attach(nullptr);
        }
        if (engine) {
            engine->Shutdown();
            engine->Release();
            engine = nullptr;
        }
        if (notify) {
            notify->Release();
            notify = nullptr;
        }
    }
};

static bool g_mf_started = false;

bool media_foundation_startup() {
    if (g_mf_started) return true;
    const HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_LITE);
    if (FAILED(hr)) {
        WP_ERROR("MFStartup failed: {:x}", static_cast<unsigned>(hr));
        return false;
    }
    g_mf_started = true;
    return true;
}

void media_foundation_shutdown() {
    if (!g_mf_started) return;
    MFShutdown();
    g_mf_started = false;
}

VideoPlayer::VideoPlayer(HWND hwnd, VideoOptions options) : m_impl(std::make_unique<Impl>()) {
    m_impl->hwnd = hwnd;
    if (!media_foundation_startup()) return;

    m_impl->notify = new MediaEngineNotify();

    IMFAttributes* attrs = nullptr;
    if (FAILED(MFCreateAttributes(&attrs, 3))) return;

    attrs->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, m_impl->notify);
    attrs->SetUINT64(MF_MEDIA_ENGINE_PLAYBACK_HWND, static_cast<UINT64>(reinterpret_cast<UINT_PTR>(hwnd)));
    attrs->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, static_cast<UINT32>(DXGI_FORMAT_B8G8R8A8_UNORM));

    IMFMediaEngineClassFactory* factory = nullptr;
  HRESULT hr = CoCreateInstance(
        CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)
    );
    if (FAILED(hr)) {
        WP_ERROR("MFMediaEngineClassFactory CoCreateInstance failed: {:x}", static_cast<unsigned>(hr));
        attrs->Release();
        return;
    }

    hr = factory->CreateInstance(0, attrs, &m_impl->engine);
    factory->Release();
    attrs->Release();

    if (FAILED(hr) || !m_impl->engine) {
        WP_ERROR("CreateInstance(MediaEngine) failed: {:x}", static_cast<unsigned>(hr));
        return;
    }

    m_impl->notify->attach(m_impl->engine);
    m_impl->engine->SetMuted(options.muted ? TRUE : FALSE);
    WP_INFO("Media Foundation engine ready for HWND {:p}", (void*)hwnd);
}

VideoPlayer::~VideoPlayer() = default;

bool VideoPlayer::load(const std::string& path) {
    if (!m_impl || !m_impl->engine) return false;
    if (path.empty()) {
        WP_ERROR("Cannot load video: path is empty");
        return false;
    }
    if (!std::filesystem::exists(path)) {
        WP_ERROR("Cannot load video: file not found: {}", path);
        return false;
    }

    const auto wide = std::filesystem::absolute(path).wstring();
    BSTR bstr = SysAllocString(wide.c_str());
    if (!bstr) return false;

    m_impl->notify->reset_loaded();
    HRESULT hr = m_impl->engine->SetSource(bstr);
    SysFreeString(bstr);
    if (FAILED(hr)) {
        WP_ERROR("SetSource failed: {:x}", static_cast<unsigned>(hr));
        return false;
    }

    hr = m_impl->engine->Load();
    if (FAILED(hr)) {
        WP_ERROR("Load failed: {:x}", static_cast<unsigned>(hr));
        return false;
    }

    for (int i = 0; i < 200; ++i) {
        if (m_impl->notify->loaded()) break;
        const auto state = m_impl->engine->GetReadyState();
        if (state >= MF_MEDIA_ENGINE_READY_HAVE_ENOUGH_DATA) break;
        Sleep(10);
    }

    // Manual seamless loop via TIMEUPDATE; MF's SetLoop(TRUE) leaves a visible gap.
    m_impl->engine->SetLoop(FALSE);
    WP_INFO("Loaded wallpaper: {}", path);
    return true;
}

void VideoPlayer::play() {
    if (m_impl && m_impl->engine) {
        m_impl->engine->Play();
    }
}

void VideoPlayer::pause() {
    if (m_impl && m_impl->engine) {
        m_impl->engine->Pause();
    }
}

void GraphicsEngine::load_video(const std::string& path, VideoOptions options) {
    std::string resolved = path;
    if (!resolved.empty()) {
        std::filesystem::path p(resolved);
        if (!p.is_absolute()) {
            p = std::filesystem::absolute(p);
        }
        resolved = p.string();
    }

    if (!m_video_player || m_video_options.muted != options.muted || m_video_hwnd != m_render_hwnd) {
        m_video_player.reset();
        m_video_player = std::make_unique<VideoPlayer>(m_render_hwnd, options);
        m_video_options = options;
        m_video_hwnd = m_render_hwnd;
    }

    if (!m_video_player) {
        WP_ERROR("Video player not initialized");
        return;
    }
    if (!m_video_player->load(resolved)) {
        return;
    }
    m_video_player->play();
}

void GraphicsEngine::release_video() {
    m_video_player.reset();
    m_video_hwnd = nullptr;
    m_video_options = VideoOptions{};
}

void GraphicsEngine::pause_video() {
    if (m_video_player) m_video_player->pause();
}

void GraphicsEngine::resume_video() {
    if (m_video_player) m_video_player->play();
}

} // namespace wp
