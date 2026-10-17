#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 inExtra;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUV;
layout(location = 3) out vec3 vWorldPos;
layout(location = 4) out vec3 vViewDir;
layout(location = 5) out float vAlpha;
layout(location = 6) out float vBillboard;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 engineState; // x=RPM, y=Temp, z=Pressure, w=Throttle
    vec4 viewFlags;   // x=exploded, y=section, z=airflow, w=time
} ubo;

void main() {
    vec4 world = ubo.model * vec4(inPosition, 1.0);

    //Camera position in world space, pulled out of the view matrix.
    vec3 camPos = (inverse(ubo.view) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;

    if(inExtra.x > 0.5){
        //Particales: build the quad in view space so it always face us.
        vec4 viewPos = ubo.view * world;
        viewPos.xy += inExtra.yz;
        gl_Position = ubo.proj * viewPos;
    }else{
        gl_Position = ubo.proj * ubo.view * world;
    }

    vColor = inColor;
    //model is a right transform, so mat3 is enough (no inverse() per vertex)
    vNormal = mat3(ubo.model) * inNormal;
    vUV     =  inTexCoord;
    vViewDir = camPos - world.xyz;
    vAlpha   = inExtra.w;
    vBillboard = inExtra.x;
}
