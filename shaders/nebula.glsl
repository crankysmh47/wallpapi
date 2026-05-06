// Star Nest by Pablo Romanandriolo
// RESTORED: Classic Stable Version

#define iterations 17
#define formuparam 0.53

#define volsteps 20
#define stepsize 0.1

#define zoom   0.800
#define tile   0.850
#define speed  0.010 

#define brightness 0.0015
#define darkmatter 0.300
#define distfading 0.730
#define saturation 0.850

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy - 0.5;
	uv.y *= iResolution.y / iResolution.x;
	vec3 dir = vec3(uv * zoom, 1.0);
	float time = iTime * speed + 0.25;

	float a1 = 0.5 + time * 0.1;
	float a2 = 0.8 + time * 0.2;
	
    // Using simple rotation matrix logic that is 100% stable in HLSL
	mat2 rot1 = mat2(cos(a1), sin(a1), -sin(a1), cos(a1));
	mat2 rot2 = mat2(cos(a2), sin(a2), -sin(a2), cos(a2));
	
	dir.xz = mul(rot1, dir.xz);
	dir.xy = mul(rot2, dir.xy);
	
	vec3 from = vec3(1.0, 0.5, 0.5);
	from += vec3(time * 2.0, time, -2.0);
	from.xz = mul(rot1, from.xz);
	from.xy = mul(rot2, from.xy);
	
	float s = 0.1;
	float fade = 1.0;
	vec3 v = vec3(0.0, 0.0, 0.0);
	
	for (int r = 0; r < volsteps; r++) {
		vec3 p = from + s * dir * 0.5;
		p = abs(vec3(tile, tile, tile) - fmod(p, vec3(tile * 2.0, tile * 2.0, tile * 2.0)));
		
		float pa = 0.0;
		float a = 0.0;
		for (int i = 0; i < iterations; i++) {
			p = abs(p) / dot(p, p) - formuparam;
			a += abs(length(p) - pa);
			pa = length(p);
		}
		
		float dm = max(0.0, darkmatter - a * a * 0.001);
		a *= a * a;
		if (r > 6) fade *= 1.0 - dm;
		
		v += vec3(fade, fade, fade);
		v += vec3(s, s*s, s*s*s*s) * a * brightness * fade;
		fade *= distfading;
		s += stepsize;
	}
	
	v = mix(vec3(length(v), length(v), length(v)), v, saturation);
	fragColor = vec4(v * 0.01, 1.0);	
}
