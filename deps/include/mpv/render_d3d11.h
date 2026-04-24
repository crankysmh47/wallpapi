#ifndef MPV_CLIENT_RENDER_D3D11_H_
#define MPV_CLIENT_RENDER_D3D11_H_

#include <d3d11.h>
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPV_RENDER_API_TYPE_D3D11 "d3d11"

#ifndef MPV_RENDER_PARAM_D3D11_DEVICE
#define MPV_RENDER_PARAM_D3D11_DEVICE ((mpv_render_param_type)6)
#endif

#ifndef MPV_RENDER_PARAM_D3D11_RENDER_TARGET
#define MPV_RENDER_PARAM_D3D11_RENDER_TARGET ((mpv_render_param_type)11)
#endif

typedef struct mpv_d3d11_render_context_params {
    ID3D11Device *device;
    ID3D11DeviceContext *context;
} mpv_d3d11_render_context_params;

typedef struct mpv_d3d11_render_target {
    ID3D11RenderTargetView *render_target_view;
} mpv_d3d11_render_target;

#ifdef __cplusplus
}
#endif

#endif
