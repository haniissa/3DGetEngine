#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec4 fragEngineState;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 engineState; // x=RPM, y=Temp, z=Pressure, w=Throttle
    vec4 viewFlags;   // x=exploded, y=section, z=airflow, w=time
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    // Pass data to fragment shader
    fragColor = inColor;
    fragNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
    fragTexCoord = inTexCoord;
    fragEngineState = ubo.engineState;

    // Pulse effect when engine is running
    float pulse = sin(ubo.viewFlags.w * 10.0 * ubo.engineState.x) * 0.1 + 1.0;
    fragColor *= pulse;
}
