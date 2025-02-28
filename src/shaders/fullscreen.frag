#version 450
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main() {
  // Currently texCoord is in clip space (locked to the camera coordinates)
  // We need to sample the position using world space coords though! so we need an inverse MVP matrix
  // The reason we need to apply an inverse matrix even though technically we never applied it already is because of how we
  // import the vertices, without buffers they come in as clip space vertices, position set using NDC.
  vec4 worldSpaceUV = inverse(PushConstants.proj * PushConstants.view * PushConstants.model) * vec4(texCoord, 1.0f, 1.0f);
  outColor = vec4(worldSpaceUV.x, worldSpaceUV.y, worldSpaceUV.z, 1.0f);
  
}
