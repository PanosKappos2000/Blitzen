#include "Renderer/Resources/Mesh/blitMeshPrimitive.h"
#include <float.h>

namespace BlitGenerator
{
	float GetLODDegradationScale(BlitzenEngine::Vertex* vtxArr, uint32_t vertexCount)
	{
		// Initializes min and max values for the bounding box of the mesh, used to track the smallest and largest points on each axis (x, y, z).
		// Set to extreme values to ensure updates with actual vertex positions.
		float meshBoundingBoxMin[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
		float meshBoundingBoxMax[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		// Loops through all vertices in the mesh and calculates the min and max positions for each axis (x, y, z) to define the bounding box that contains the mesh.
		for (uint32_t vtx = 0; vtx < vertexCount; ++vtx)
		{
			float* vtxAxisArr = &vtxArr[vtx].position.x;

			// Updates the min and max for each axis(x, y, z) to find the bounds of the mesh.
			for (uint32_t axis = 0; axis < 3; ++axis)
			{
				float vtxAxis = vtxAxisArr[axis];

				meshBoundingBoxMin[axis] = meshBoundingBoxMin[axis] > vtxAxis ? vtxAxis : meshBoundingBoxMin[axis];
				meshBoundingBoxMax[axis] = meshBoundingBoxMax[axis] < vtxAxis ? vtxAxis : meshBoundingBoxMax[axis];
			}
		}

		float extent = 0.f;

		// Finds the largest difference between the max and min values along each axis.
		// The extent is the largest size of the bounding box in world space.
		extent = (meshBoundingBoxMax[0] - meshBoundingBoxMin[0]) < extent ? extent : (meshBoundingBoxMax[0] - meshBoundingBoxMin[0]);
		extent = (meshBoundingBoxMax[1] - meshBoundingBoxMin[1]) < extent ? extent : (meshBoundingBoxMax[1] - meshBoundingBoxMin[1]);
		extent = (meshBoundingBoxMax[2] - meshBoundingBoxMin[2]) < extent ? extent : (meshBoundingBoxMax[2] - meshBoundingBoxMin[2]);

		return extent;
	}
}