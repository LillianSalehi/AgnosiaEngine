#extension GL_EXT_buffer_reference : require // BDA
#extension GL_EXT_scalar_block_layout : require // Sane buffer layout
#extension GL_EXT_nonuniform_qualifier : require // Descriptor Indexing

struct Vertex {
    vec3 pos;
    vec3 normal;
    vec3 color;
    vec2 texCoord;
}; 

layout(buffer_reference, scalar) readonly buffer VertexBuffer { 
	Vertex vertices[];
};
layout(buffer_reference, scalar) readonly buffer GPUBuffer { 
    VertexBuffer vertBuffer;
    vec3 objPos;
    vec3 lightPos;
    vec3 lightColor;
    vec3 camPos;
    int diffuseID;
    int metallicID;
    int aoID;
    int roughnessID;
    mat4 model;
    mat4 view;
    mat4 proj;
};
layout( push_constant, scalar ) uniform constants {
    GPUBuffer gpuBuffer;
};
