#include "blitObject.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenEngine
{
    void CreateTestGameObjects(RenderingResources& resources, uint32_t drawCount)
    {
        resources.objectCount = drawCount;// Normally the draw count differs from the game object count, but the engine is really simple at the moment
        resources.transforms.Resize(resources.objectCount);// Every object has a different transform
        // Hardcode a large amount of male model mesh
        for(size_t i = 0; i < resources.objectCount / 10; ++i)
        {
            BlitzenEngine::MeshTransform& transform = resources.transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'500 - 50,//x 
            (float(rand()) / RAND_MAX) * 1'500 - 50,//y
            (float(rand()) / RAND_MAX) * 1'500 - 50);//z
            transform.pos = translation;
            transform.scale = 0.1f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
            (float(rand()) / RAND_MAX) * 2 - 1, // y
            (float(rand()) / RAND_MAX) * 2 - 1); // z
		    float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = resources.objects[i];

            currentObject.meshIndex = 3;// Hardcode the bunny mesh for each object in this loop
            currentObject.transformIndex = static_cast<uint32_t>(i);// Transform index is the same as the object index
        }
        // Hardcode a large amount of objects with the high polygon kitten mesh and random transforms
        for (size_t i = resources.objectCount / 10; i < resources.objectCount / 8; ++i)
        {
            BlitzenEngine::MeshTransform& transform = resources.transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'500 - 50,//x 
            (float(rand()) / RAND_MAX) * 1'500 - 50,//y
            (float(rand()) / RAND_MAX) * 1'500 - 50);//z
            transform.pos = translation;
            transform.scale = 1.f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
            (float(rand()) / RAND_MAX) * 2 - 1, // y
            (float(rand()) / RAND_MAX) * 2 - 1); // z
		    float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = resources.objects[i];

            currentObject.meshIndex = 1;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = static_cast<uint32_t>(i);// Transform index is the same as the object index
        }
        // Hardcode a large amount of stanford dragons
        for (size_t i = resources.objectCount / 8; i < resources.objectCount / 6; ++i)
        {
            BlitzenEngine::MeshTransform& transform = resources.transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'500 - 50,//x 
                (float(rand()) / RAND_MAX) * 1'500 - 50,//y
                (float(rand()) / RAND_MAX) * 1'500 - 50);//z
            transform.pos = translation;
            transform.scale = 0.1f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
            float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = resources.objects[i];

            currentObject.meshIndex = 0;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = static_cast<uint32_t>(i);// Transform index is the same as the object index
        }
        // Hardcode a large amount of standford bunnies
        for (size_t i = resources.objectCount / 6; i < resources.objectCount; ++i)
        {
            BlitzenEngine::MeshTransform& transform = resources.transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'500 - 50,//x 
                (float(rand()) / RAND_MAX) * 1'500 - 50,//y
                (float(rand()) / RAND_MAX) * 1'500 - 50);//z
            transform.pos = translation;
            transform.scale = 5.f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
            float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = resources.objects[i];

            currentObject.meshIndex = 2;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = static_cast<uint32_t>(i);// Transform index is the same as the object index
        }
    }
}