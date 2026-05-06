/*
  Wallpapi Shader Compatibility Header
  Maps Shadertoy GLSL to HLSL for D3DCompile
*/

typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;
typedef float2x2 mat2;
typedef float3x3 mat3;
typedef float4x4 mat4;

#define mix lerp
#define fract frac
#define mod fmod

cbuffer PerFrame : register(b0) {
    float3 iResolution;
    float  iTime;
    float  iTimeDelta;
    int    iFrame;
    float4 iMouse;
    float4 iPrevMouse;
};

// Ping-pong simulation texture (previous frame state)
Texture2D    iChannel0        : register(t0);
SamplerState iChannel0Sampler : register(s0);

// GLSL-style texture() wrapper
#define texture(tex, uv) iChannel0.Sample(iChannel0Sampler, uv)

// Simple VS output
struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// User shader will be appended after this
// It must implement: void mainImage(out vec4 fragColor, in vec2 fragCoord)
void mainImage(out vec4 fragColor, in vec2 fragCoord);

vec4 main(VS_OUTPUT input) : SV_Target {
    vec4 fragColor = vec4(0, 0, 0, 1);
    vec2 fragCoord = input.uv * iResolution.xy;
    mainImage(fragColor, fragCoord);
    return fragColor;
}
