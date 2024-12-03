#version 450
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    Vertex vertex = PushConstants.vertBuffer.vertices[gl_VertexIndex];
    
    gl_Position = PushConstants.proj * PushConstants.view * PushConstants.model * 
                    vec4(vertex.pos + PushConstants.objPos, 1.0f);
    fragColor = vertex.color.rgb;
    fragTexCoord = vertex.texCoord;
}
