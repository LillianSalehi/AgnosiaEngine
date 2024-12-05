#version 450
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(binding = 1) uniform sampler2D texSampler[];


layout(location = 0) in vec3 v_norm;
layout(location = 1) in vec3 v_pos;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main() {
  vec3 diffuseColor = texture(texSampler[PushConstants.textureID], texCoord).rgb;
  vec3 ambientColor = vec3(0.05f,0.05f, 0.05f) * diffuseColor;
  float lightPower = 5;
  vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
  float cosTheta = dot(PushConstants.lightPos, v_norm);
  float sqrDist = distance(v_pos, PushConstants.lightPos)*distance(v_pos, PushConstants.lightPos);
  outColor = vec4(ambientColor + clamp(diffuseColor * lightColor * lightPower * cosTheta / sqrDist, vec3(0,0,0), vec3(1,1,1)), 1.0f);
}
