#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UBO{
    mat4 M, V, P;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 texCoord;


void main() {
    gl_Position = P * V * M * vec4(inPosition, 1.0);
    fragColor = inColor;
    texCoord = inTexCoord;
}