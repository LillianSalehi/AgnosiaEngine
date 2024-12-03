#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

struct Vertex {
    vec3 pos;
    vec3 color;
    vec2 texCoord;
}; 

layout(buffer_reference, scalar) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};
layout( push_constant, scalar ) uniform constants {
    VertexBuffer vertBuffer;
    vec3 objPos;
    int textureID;
    mat4 model;
    mat4 view;
    mat4 proj;
} PushConstants;
