#version 460 core
#include "common.glsl"

layout(location = 2) out vec2 texCoord;

 void main() 
{
    texCoord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(texCoord * 2.0f + -1.0f, 1.0f, 1.0f);
}
