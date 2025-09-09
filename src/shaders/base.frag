#version 460 core
#include "common.glsl"

layout(set = 0, binding = 1) uniform texture2D _texture[];
layout(set = 1, binding = 2) uniform sampler _sampler;

layout(location = 0) in vec3 v_norm;
layout(location = 1) in vec3 v_pos;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

// Trowbridge-Reitz GGX NDF- Approximate the relative surface area of microfacets exactly aligned to the halfway vector.
vec3 DistributionTRGGX(vec3 N, vec3 H, vec3 roughness) {
  vec3 a = roughness*roughness;
  vec3 a2 = a*a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH*NdotH;

  vec3 num = a2;
  vec3 denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = 3.14159 * denom * denom;

  return num / denom;
}
// Schlick GGX, Approximate overshadowed microfacets occlusion. 
vec3 GeometrySchlickGGX(float NdotV, vec3 roughness) {
  vec3 r = (roughness + vec3(1.0));
  vec3 k = (r*r) / 8.0;

  float num = NdotV;  
  vec3 denom = NdotV * (1.0 - k) + k;
	
  return num / denom;
}
// Smith's method- take into account both view direction and light direction.
vec3 GeometrySmith(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 k) {
  float NdotV = max(dot(normal, viewDir), 0.0);
  float NdotL = max(dot(normal, lightDir), 0.0);
  vec3 ggx1 = GeometrySchlickGGX(NdotV, k);
  vec3 ggx2 = GeometrySchlickGGX(NdotL, k);

  return ggx1 * ggx2;
}
// Fresnel-Schlick equation- approximate base reflectivity at grazing angles.
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
  const float PI = 3.14159265359;

  vec3 lightColor = gpuBuffer.lightColor;
  vec3 albedo = texture(sampler2D(_texture[gpuBuffer.diffuseID], _sampler), texCoord).rgb;
  vec3 metallic = texture(sampler2D(_texture[gpuBuffer.metallicID], _sampler), texCoord).rgb;
  vec3 ao = texture(sampler2D(_texture[gpuBuffer.aoID], _sampler), texCoord).rgb;
  vec3 roughness = texture(sampler2D(_texture[gpuBuffer.roughnessID], _sampler), texCoord).rgb;
  
  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, albedo, metallic);

  vec3 N = normalize(v_norm);
  vec3 V = normalize(gpuBuffer.camPos - v_pos);

  vec3 Lo = vec3(0.0);

  // iterate over each light
  for(int i = 0; i < 1; ++i) {
    vec3 L = normalize(gpuBuffer.lightPos - v_pos);
    vec3 H = normalize(V+L);

    float distance = length(gpuBuffer.lightPos - v_pos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightColor * attenuation;
      
    // Cook-Torrance BRDF
    vec3 NDF = DistributionTRGGX(N, H, roughness);       
    vec3 G = GeometrySmith(N, V, L, roughness);       
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  vec3 ambient = vec3(0.03) * albedo * ao;
  vec3 color = ambient + Lo;

  // Tonemap using the Reinhard operator to gamma color space
  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0/2.2));

  outColor = vec4(color, 1.0);
}

