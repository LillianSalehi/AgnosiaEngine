#version 450
#extension GL_EXT_buffer_reference : require

layout(binding = 0) uniform UniformBufferObject {
    float time;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
struct Vertex {

	vec3 pos;
    vec3 color;
    vec2 texCoord;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};
layout( push_constant ) uniform constants {
    VertexBuffer vertBuffer;
}PushConstants;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    Vertex v = PushConstants.vertBuffer.vertices[gl_VertexIndex];
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(v.pos, 1.0f);
    fragColor = v.color.xyz;
    fragTexCoord = v.texCoord;
}
