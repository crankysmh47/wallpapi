// [STATEFUL]
// Navier-Stokes Fluid Simulation (Physics Pass)
// RG = Velocity, B = Pressure, A = Ink/Density

#define res iResolution.xy

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / res;
    vec2 p = (fragCoord - 0.5 * res) / res.y;
    
    // 1. Advection (move properties along velocity field)
    vec4 oldData = texture(iChannel0, uv);
    vec2 vel = oldData.xy;
    
    // Sample the position where the fluid came from
    vec2 back_uv = uv - vel * 0.001; 
    vec4 advected = texture(iChannel0, back_uv);
    
    vel = advected.xy;
    float press = advected.z;
    float ink = advected.w;
    
    // 2. Mouse Interaction (Dragging)
    vec2 m = (iMouse.xy - 0.5 * res) / res.y;
    vec2 pm = (iPrevMouse.xy - 0.5 * res) / res.y;
    vec2 mouseVel = (m - pm) * 20.0;
    
    float mouseDist = length(p - m);
    float strength = smoothstep(0.1, 0.0, mouseDist);
    
    // Drag the fluid with the mouse velocity
    vel += mouseVel * strength * 0.5;
    ink += length(mouseVel) * strength * 2.0;
    
    // Subtle continuous ink injection
    ink += strength * 0.01;
    
    // 3. Simple Pressure/Divergence logic
    vec2 v_left  = texture(iChannel0, uv + vec2(-1.0, 0.0)/res).xy;
    vec2 v_right = texture(iChannel0, uv + vec2(1.0, 0.0)/res).xy;
    vec2 v_up    = texture(iChannel0, uv + vec2(0.0, 1.0)/res).xy;
    vec2 v_down  = texture(iChannel0, uv + vec2(0.0, -1.0)/res).xy;
    
    float div = (v_right.x - v_left.x + v_up.y - v_down.y) * 0.5;
    press = mix(press, div, 0.1);
    vel -= vec2(v_right.x - v_left.x, v_up.y - v_down.y) * press * 0.05;

    // 4. Damping (fade out energy)
    vel *= 0.985;
    ink *= 0.992;
    
    if (iFrame < 5) {
        fragColor = vec4(0,0,0,0);
    } else {
        fragColor = vec4(vel, press, ink);
    }
}
