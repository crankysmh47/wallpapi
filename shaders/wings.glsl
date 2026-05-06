// Wings of Freedom (AOT Theme)
// Abstract glowing wings (Scout Regiment)

#define time iTime
#define resolution iResolution.xy

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord.xy / resolution.xy) * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;

    float t = time * 0.5;
    vec3 col = vec3(0.05, 0.05, 0.1); // Dark blue background

    // Symmetry for wings
    vec2 p = vec2(abs(uv.x), uv.y);
    
    // Feather-like shapes
    float wings = 0.0;
    for(float i=1.0; i<8.0; i++)
    {
        float offset = i * 0.15;
        float wing_shape = smoothstep(0.1, 0.0, abs(p.x - 0.2 - p.y * 0.5 + offset));
        wing_shape *= smoothstep(0.5, -0.5, p.y); // Height limit
        wings += wing_shape * (1.0 / i);
    }
    
    // Blue and White coloring
    vec3 wing_col = mix(vec3(0.0, 0.3, 0.8), vec3(1.0, 1.0, 1.0), wings);
    col += wing_col * wings * 1.5;
    
    // Sparkles/Particles
    float particles = frac(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
    if(particles > 0.995) col += vec3(1.0, 1.0, 1.0) * abs(sin(t + particles * 10.0));

    fragColor = vec4(col, 1.0);
}
