#version 450
// inPosition and inColor are vertex attributes, per-vertex properties defined in the vertex buffer!
// Layout assigns indices to access these inputs, dvec3 takes 2 slots so we must index it at 2. https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
