// Plasma Globe by nimitz
// https://www.shadertoy.com/view/XsjXRm

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    uv = uv * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    float t = iTime * 0.5;
    float r = length(uv);
    float a = atan2(uv.y, uv.x);

    float d = 0.0;
    for(float i=1.0; i<5.0; i++)
    {
        d += sin(uv.x*i*2.0 + t + sin(t*i)) * 0.5 + 0.5;
        d += sin(uv.y*i*2.5 - t*0.8 + cos(t*i*1.1)) * 0.5 + 0.5;
    }
    d /= 8.0;

    vec3 col = vec3(0.1, 0.2, 0.5) * d;
    col += vec3(0.8, 0.4, 0.2) * (1.0 - abs(sin(r*10.0 - t*3.0)));
    col *= (1.0 - r*0.5);

    fragColor = vec4(col, 1.0);
}
