// Fluid - Mouse Reactive Curl Noise
// Single-pass approximation of Navier-Stokes style fluid
// Mouse drags "inject" velocity into the flow field

#define TAU 6.28318530718

// --- Noise Primitives ---
float hash(vec2 p) {
    p = frac(p * vec2(127.1, 311.7));
    p += dot(p, p + 19.19);
    return frac(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f); // smoothstep
    return mix(
        mix(hash(i + vec2(0,0)), hash(i + vec2(1,0)), f.x),
        mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), f.x),
        f.y
    );
}

// Fractional Brownian Motion - layered noise
float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));
    for(int i = 0; i < 6; i++) {
        v += a * noise(p);
        p  = mul(rot, p) * 2.0 + vec2(7.3, 1.9);
        a *= 0.5;
    }
    return v;
}

// Curl of the noise field = divergence-free velocity (like real fluid)
vec2 curl(vec2 p, float eps) {
    float n1 = fbm(p + vec2(0.0, eps));
    float n2 = fbm(p - vec2(0.0, eps));
    float n3 = fbm(p + vec2(eps, 0.0));
    float n4 = fbm(p - vec2(eps, 0.0));
    float dx = (n1 - n2) / (2.0 * eps);
    float dy = (n3 - n4) / (2.0 * eps);
    return vec2(dx, -dy); // Perpendicular = curl
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 res = iResolution.xy;
    vec2 uv  = fragCoord / res;

    // Aspect-correct space, centered at 0
    vec2 p   = (fragCoord - 0.5 * res) / res.y;
    vec2 m   = (iMouse.xy  - 0.5 * res) / res.y;

    float t = iTime * 0.15;

    // --- Mouse Influence ---
    // Vector from pixel to mouse
    vec2 toMouse  = m - p;
    float mouseDist = length(toMouse);
    
    // Mouse velocity approximation using iMouse.zw (click coords = prev mouse)
    // We fake "drag velocity" as normalized direction weighted by proximity
    float mouseStrength = smoothstep(0.5, 0.0, mouseDist) * 1.5;
    vec2  mouseVel     = normalize(toMouse + vec2(0.001)) * mouseStrength;

    // --- Curl Noise Flow Field ---
    // Scale the space for nice large swirls
    vec2 flowP = p * 2.0 + vec2(t * 0.3, t * 0.1);
    
    // Advect the sample point along the curl field (fake advection = time-warp)
    for(int i = 0; i < 4; i++) {
        vec2 c = curl(flowP + t, 0.01);
        flowP += c * 0.3;
    }

    // Inject mouse velocity into the flow
    flowP += mouseVel * 0.4;

    // --- Sample FBM for color ---
    float n = fbm(flowP);
    float n2 = fbm(flowP * 1.5 + 3.0);

    // --- Color Palette (deep blue/cyan, cool fluid feel) ---
    vec3 col_a = vec3(0.02, 0.08, 0.25);  // deep ocean blue
    vec3 col_b = vec3(0.0,  0.5,  0.9);   // bright cyan
    vec3 col_c = vec3(0.6,  0.9,  1.0);   // white crest

    vec3 col = col_a;
    col = mix(col, col_b, smoothstep(0.3, 0.7, n));
    col = mix(col, col_c, smoothstep(0.65, 0.85, n));

    // Secondary turbulence layer
    col = mix(col, col_b * 0.5 + col_c * 0.5, n2 * 0.3);

    // --- Mouse Glow ---
    float glow = smoothstep(0.25, 0.0, mouseDist);
    col += vec3(0.2, 0.6, 1.0) * pow(glow, 2.0) * 1.5;

    // Subtle vignette
    float vig = 1.0 - smoothstep(0.5, 1.2, length(p));
    col *= vig;

    fragColor = vec4(col, 1.0);
}
