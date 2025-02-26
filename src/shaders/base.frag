#version 450
#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(binding = 1) uniform sampler2D texSampler[];


layout(location = 0) in vec3 v_norm;
layout(location = 1) in vec3 v_pos;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

void main() {
  float lightPower = 1;
  vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
  
  vec3 objectColor = texture(texSampler[PushConstants.textureID], texCoord).rgb;
  vec3 ambient = PushConstants.ambient * vec3(0.2f, 0.2f, 0.2f);
  
  vec3 reflectPos = reflect(-PushConstants.lightPos, v_norm);
  float spec = pow(max(dot(normalize(PushConstants.camPos), normalize(reflectPos)), 0.0), PushConstants.shine);
  vec3 specular = PushConstants.spec * spec * vec3(1.0f, 1.0f, 1.0f);

  // Lambertian reflectance model; Diffuse = (LightDirection⋅Normal)(Color)(Intensity)
  float lightDotNorm = max(dot(normalize(PushConstants.lightPos), normalize(v_norm)), 0.0f);
  vec3 diffuse = lightDotNorm * lightColor * vec3(0.5f, 0.5f, 0.5f);
  // Inverse Square law; light intensity falls off proportionally to the the distance from the light source.
  // This will be implemented for point lights, right now lights act as undirected suns, complete uniform brightness regardless of distance and angle.
  // Note; we multiply by hand here rather than use pow() because pow() is only more/equally performant than by hand multiplication after 8 iterations.
  //float sqrDist = distance(v_pos, PushConstants.lightPos)*distance(v_pos, PushConstants.lightPos);
  
  outColor = vec4((ambient + diffuse + specular) * objectColor, 1.0f);
  
}
