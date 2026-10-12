#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec4 fragEngineState;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple directional lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, lightDir), 0.0);

    // Ambient + diffuse
    vec3 ambient = fragColor * 0.3;
    vec3 diffuse = fragColor * diff * 0.7;

    // Heat glow for combustor (based on temperature)
    float heat = fragEngineState.y;
    vec3 heatColor = vec3(1.0, 0.4, 0.1) * heat * 0.5;

    outColor = vec4(ambient + diffuse + heatColor, 1.0);
}
