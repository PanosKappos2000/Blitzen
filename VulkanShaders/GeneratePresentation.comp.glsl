#version 460

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1)in;

layout(push_constant) uniform Constants 
{
    vec2 imageExtent;
};

layout (set = 0, binding = 0) uniform writeonly image2D presentImage;
layout (set = 0, binding = 1) uniform sampler2D colorImage;

void main()
{
    uvec2 pos = gl_GlobalInvocationID.xy;
	vec2 uv = (vec2(pos) + 0.5) / imageExtent;
	vec3 color = texture(colorImage, uv).rgb;
	imageStore(presentImage, ivec2(pos), vec4(color, 1.0));
}