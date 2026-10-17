#version 450

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec3 vWorldPos;
layout(location = 4) in vec3 vViewDir;
layout(location = 5) in float vAlpha;
layout(location = 6) in float vBillboard;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 engineState; // x = rpm(0..1), y = T3/2000,w = throttle
    vec4 viewFlags;  // x = exploded, y = section, z  = airflow, w = time
} ubo;


const vec3 KEY_DIR  = vec3( 0.4364,  0.5951,  0.2777);
const vec3 FILL_DIR = vec3(-0.8111,  0.1248, -0.4991);
const vec3 RIM_DIR  = vec3(-0.1985,  0.2977,  0.9130);

const float ENGINE_LENGTH = 5.8;

//Rough temperature profile along the engine axis, normalised to 0..1
float heatProfile(float t) {
    if(t < 0.35) return  mix(0.03, 0.20, t / 0.35);
    if(t < 0.52) return mix(0.20, 1.00, (t - 0.35) / 0.17);
    if(t < 0.70) return mix(1.00, 0.45, (t - 0.52) / 0.18);
    return mix(0.45, 0.12, (t - 0.70) / 0.30);
}

void main(){
    //-----Section view: cut the casing in half along +Z-----
    if(ubo.viewFlags.y > 0.5 && vWorldPos.z > 0.0) discard;

    //-----Prticles: soft round sprite, drawn additively-----
    if(vBillboard > 0.5){
        vec2 d = vUV * 2.0 - 1.0;
        float r2 = dot(d, d);
        if(r2 > 1.0) discard; // round off the quad corners
        float core = exp(-r2 * 4.0); // soft gaussian falloff
        vec3 c = mix(vColor, vec3(1.0), core * 0.55);
        outColor = vec4(c, core * vAlpha);
        return;
    }

    //Metal shading ------------------------
    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N;  // lathe shells are single sided
    vec3 V = normalize(vViewDir);


    float ndv = max(dot(N, V), 0.0);
    float fresnel = pow(1.0 - ndv, 3.0);

    //Heat follows position along the engine, scaled by RPM so a shut-down
    // engine is cold and dark.
    float t = clamp(vWorldPos.x / ENGINE_LENGTH, 0.0, 1.0);
    float heat = heatProfile(t) * ubo.engineState.x;
    vec3 heatColor = mix(vec3(1.0, 0.25, 0.25), vec3(1.0, 0.72, 0.32), heat);

    vec3 base = mix(vColor, heatColor, clamp(heat, 0.0, 1.0) * 0.8);


    vec3 lit = base * 0.5;
    lit += base * max(dot(N, KEY_DIR), 0.0) * 0.90;
    lit += base * max(dot(N, FILL_DIR), 0.0) * 0.30;
    lit += base * max(dot(N,  RIM_DIR), 0.0) * 0.16;

    lit += base * pow(max(dot(N, normalize(KEY_DIR + V)), 0.0), 72.0) * 1.30;
    lit += vec3(1.0) * pow(max(dot(N, normalize(RIM_DIR + V)), 0.0), 28.0) * 0.22;
    lit += vec3(0.32, 0.52, 0.95) * fresnel * 0.75;
    lit += heatColor * heat * 1.6;

    //Reinhard tone map. The swapchain is an _SRGB format, so the hardware
    // applies linear -> sRGB for us: do NOT gamma here or it double-applies.
    lit = lit / (lit + vec3(1.0));

    outColor = vec4(lit, 1.0);
}