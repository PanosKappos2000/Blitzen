#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout (set = 0, binding = 0, r32f) uniform writeonly image2D outImage;
layout (set = 0, binding = 1) uniform sampler2D inImage;

layout(push_constant) uniform constants
{
    vec2 imageSize;
};

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;

    float depth = texture(inImage, (vec2(pos) + vec2(0.5)) / imageSize).x;

    imageStore(outImage, ivec2(pos), vec4(depth));
}