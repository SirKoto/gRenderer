#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SceneUBO{
    mat4 V, P;
};
layout(set = 2, binding = 0) uniform ObjectUBO{
    mat4 M;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
//layout(location = 2) in vec3 inColor;
//layout(location = 3) in vec2 inTexCoord;


layout(location = 0) out vec3 fragColor;
//layout(location = 1) out vec2 texCoord;

void main() {
    gl_Position = P * V * M * vec4(inPosition, 1.0);
    
    vec3 wNorm = normalize(vec3(M * vec4(inNormal, 0)));

    float d = dot(wNorm, vec3(0.408, 0.408, 0.81));
    float k = 0.5 + 0.6 * (d > 0 ? d : 0.0);
    fragColor = k * (0.5 + inNormal * 0.5);
    //texCoord = inTexCoord;
}