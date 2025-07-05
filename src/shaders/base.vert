#version 460 core
#include "common.glsl"

layout(location = 0) out vec3 v_norm;
layout(location = 1) out vec3 v_pos;
layout(location = 2) out vec2 texCoord;


void main() {
    Vertex vertex = PushConstants.vertBuffer.vertices[gl_VertexIndex];
    
    gl_Position = PushConstants.proj * PushConstants.view * PushConstants.model * 
                    vec4(vertex.pos + PushConstants.objPos, 1.0f);
                    
    v_norm = vertex.normal;
    v_pos = vertex.pos;
    texCoord = vertex.texCoord;
}
