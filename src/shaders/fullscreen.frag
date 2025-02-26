#version 450
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main() {
    
  outColor = vec4(1.0f,1.0f,1.0f, 0.1f);
  
}
