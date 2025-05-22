#version 450

layout (location = 0) in vec3 fragNormal;

out vec4 finalColor;

void main()
{
    finalColor = vec4(fragNormal, 1.0);
}