#version 450

void main()
{
    // Promotes the bounding sphere's center to model and the view coordinates (frustum culling will be done on view space)
    //center = RotateQuat(boundCenter, orientation) * scale + pos;
    //vec3 center = (view * vec4(center, 1)).xyz;
	//float radius = boundRadius * scale;
}